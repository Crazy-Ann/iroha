/**
 * Copyright Soramitsu Co., Ltd. 2017 All Rights Reserved.
 * http://soramitsu.co.jp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "crypto/hash.hpp"
#include "framework/test_subscriber.hpp"
#include "main/application.hpp"
#include "main/raw_block_insertion.hpp"
#include "model/generators/block_generator.hpp"
#include "module/irohad/ametsuchi/ametsuchi_fixture.hpp"

using namespace framework::test_subscriber;

class TestIrohad : public Irohad {
 public:
  TestIrohad(const std::string &block_store_dir,
             const std::string &redis_host,
             size_t redis_port,
             const std::string &pg_conn,
             size_t torii_port,
             uint64_t peer_number)
      : Irohad(block_store_dir,
               redis_host,
               redis_port,
               pg_conn,
               torii_port,
               peer_number) {}

  auto &getCommandService() { return command_service; }

  auto &getPeerCommunicationService() { return pcs; }

  auto &getCryptoProvider() { return crypto_verifier; }

  void run() override {
    grpc::ServerBuilder builder;
    int port = 0;
    builder.AddListeningPort(
        peer.address, grpc::InsecureServerCredentials(), &port);
    builder.RegisterService(ordering_init.ordering_gate_transport.get());
    builder.RegisterService(ordering_init.ordering_service.get());
    builder.RegisterService(yac_init.consensus_network.get());
    builder.RegisterService(loader_init.service.get());
    internal_server = builder.BuildAndStart();
    internal_thread = std::thread([this] { internal_server->Wait(); });
    log_->info("===> iroha initialized");
    loop_thread = std::thread([this] { loop->run(); });
  }

  ~TestIrohad() override {
    if (loop) {
      loop->stop();
      loop->close();
    }
    if (loop_thread.joinable()) {
      loop_thread.join();
    }
  }

  std::thread loop_thread;
};

class TxPipelineIntegrationTest : public iroha::ametsuchi::AmetsuchiTest {
 public:
  TxPipelineIntegrationTest() {
    spdlog::set_level(spdlog::level::off);
    irohad = std::make_shared<TestIrohad>(
        block_store_path, redishost_, redisport_, pgopt_, 0, 0);
  }

  std::shared_ptr<TestIrohad> irohad;

  std::condition_variable cv;
  std::mutex m;

  std::vector<iroha::model::Proposal> proposals;
  std::vector<iroha::model::Block> blocks;
};

TEST_F(TxPipelineIntegrationTest, TxPipelineTest) {
  ASSERT_TRUE(irohad->storage);

  using namespace std::chrono_literals;

  // insert genesis block
  iroha::main::BlockInserter inserter(irohad->storage);

  auto genesis_block =
      iroha::model::generators::BlockGenerator().generateGenesisBlock(
          {"0.0.0.0:10000"});
  inserter.applyToLedger({genesis_block});

  // initialize irohad
  irohad->init();

  // generate test command
  auto cmd =
      iroha::model::generators::CommandGenerator().generateAddAssetQuantity(
          "test@test",
          "coin#test",
          iroha::Amount().createFromString("20.00").value());

  // generate test transaction
  auto ts = std::chrono::system_clock::now().time_since_epoch() / 1ms;
  auto tx =
      iroha::model::generators::TransactionGenerator().generateTransaction(
          ts, "admin@test", 1, {cmd});
  tx.signatures.emplace_back();

  // generate expected proposal
  iroha::model::Proposal proposal({tx});
  proposal.height = 2;

  // generate expected block
  iroha::model::Block block{};
  block.height = proposal.height;
  block.prev_hash = genesis_block.hash;
  block.transactions = {tx};
  block.txs_number = 1;
  block.created_ts = 0;
  block.merkle_root.fill(0);
  block.hash = iroha::hash(block);
  block = irohad->getCryptoProvider()->sign(block);

  // verify proposal
  auto proposal_wrapper = make_test_subscriber<CallExact>(
      irohad->getPeerCommunicationService()->on_proposal(), 1);
  proposal_wrapper.subscribe(
      [this](auto proposal) { proposals.push_back(proposal); });

  // verify commit and block
  auto commit_wrapper = make_test_subscriber<CallExact>(
      irohad->getPeerCommunicationService()->on_commit(), 1);
  commit_wrapper.subscribe([this](auto commit) {
    auto block_wrapper = make_test_subscriber<CallExact>(commit, 1);
    block_wrapper.subscribe([this](auto block) { blocks.push_back(block); });
  });
  irohad->getPeerCommunicationService()->on_commit().subscribe(
      [this](auto) { cv.notify_one(); });

  // start irohad
  irohad->run();

  // send transaction to torii
  auto pb_tx = iroha::model::converters::PbTransactionFactory().serialize(tx);

  google::protobuf::Empty response;
  irohad->getCommandService()->ToriiAsync(pb_tx, response);

  // wait for commit
  std::unique_lock<std::mutex> lk(m);

  cv.wait_for(lk, 10s, [this] { return blocks.size() == 1; });

  ASSERT_TRUE(proposal_wrapper.validate());
  ASSERT_EQ(1, proposals.size());
  ASSERT_EQ(proposal, proposals.front());

  ASSERT_TRUE(commit_wrapper.validate());
  ASSERT_EQ(1, blocks.size());
  ASSERT_EQ(block, blocks.front());
}
