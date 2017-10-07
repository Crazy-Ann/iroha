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

#include "ametsuchi/impl/storage_impl.hpp"

#include "ametsuchi/impl/flat_file_block_query.hpp"
#include "ametsuchi/impl/mutable_storage_impl.hpp"
#include "ametsuchi/impl/postgres_wsv_query.hpp"
#include "ametsuchi/impl/temporary_wsv_impl.hpp"
#include "model/converters/json_common.hpp"

namespace iroha {
  namespace ametsuchi {

    StorageImpl::StorageImpl(
        const Config::Redis &redisConf,
        const Config::Postgres &pgConf,
        const Config::BlockStorage &storeConf,
        std::unique_ptr<FlatFile> block_store,
        std::unique_ptr<cpp_redis::redis_client> index,
        std::unique_ptr<pqxx::lazyconnection> wsv_connection,
        std::unique_ptr<pqxx::nontransaction> wsv_transaction)
        : redisConfig_(redisConf),
          postgresConfig_(pgConf),
          storeConfig_(storeConf),
          block_store_(std::move(block_store)),
          index_(std::move(index)),
          wsv_connection_(std::move(wsv_connection)),
          wsv_transaction_(std::move(wsv_transaction)),
          wsv_(std::make_shared<PostgresWsvQuery>(*wsv_transaction_)),
          blocks_(std::make_shared<FlatFileBlockQuery>(*block_store_)) {
      log_ = logger::log("StorageImpl");

      wsv_transaction_->exec(init_);
      wsv_transaction_->exec(
          "SET SESSION CHARACTERISTICS AS TRANSACTION READ ONLY;");
    }

    std::unique_ptr<TemporaryWsv> StorageImpl::createTemporaryWsv() {
      auto command_executors = model::CommandExecutorFactory::create();
      if (not command_executors.has_value()) {
        log_->error("Cannot create CommandExecutorFactory");
        return nullptr;
      }

      auto postgres_connection =
          std::make_unique<pqxx::lazyconnection>(postgresConfig_.options());

      try {
        postgres_connection->activate();
      } catch (const pqxx::broken_connection &e) {
        log_->error("Connection to PostgreSQL broken: {}", e.what());
        return nullptr;
      }
      auto wsv_transaction = std::make_unique<pqxx::nontransaction>(
          *postgres_connection, "TemporaryWsv");

      return std::make_unique<TemporaryWsvImpl>(
          std::move(postgres_connection),
          std::move(wsv_transaction),
          std::move(command_executors.value()));
    }

    std::unique_ptr<MutableStorage> StorageImpl::createMutableStorage() {
      auto command_executors = model::CommandExecutorFactory::create();
      if (not command_executors.has_value()) {
        log_->error("Cannot create CommandExecutorFactory");
        return nullptr;
      }

      auto postgres_connection =
          std::make_unique<pqxx::lazyconnection>(postgresConfig_.options());
      try {
        postgres_connection->activate();
      } catch (const pqxx::broken_connection &e) {
        log_->error("Connection to PostgreSQL broken: {}", e.what());
        return nullptr;
      }
      auto wsv_transaction = std::make_unique<pqxx::nontransaction>(
          *postgres_connection, "TemporaryWsv");

      auto index = std::make_unique<cpp_redis::redis_client>();
      try {
        index->connect(redisConfig_.host, redisConfig_.port);
      } catch (const cpp_redis::redis_error &e) {
        log_->error("Connection to Redis broken: {}", e.what());
        return nullptr;
      }

      nonstd::optional<hash256_t> top_hash;

      blocks_->getTopBlocks(1)
          .subscribe_on(rxcpp::observe_on_new_thread())
          .as_blocking()
          .subscribe([&top_hash](auto block) { top_hash = block.hash; });

      return std::make_unique<MutableStorageImpl>(
          top_hash.value_or(hash256_t{}),
          std::move(index),
          std::move(postgres_connection),
          std::move(wsv_transaction),
          std::move(command_executors.value()));
    }

    nonstd::optional<ConnectionContext> StorageImpl::initConnections(
        const Config::Redis &redis,
        const Config::Postgres &pg,
        const Config::BlockStorage &store) {
      auto log_ = logger::log("StorageImpl:initConnection");
      log_->info("Start storage creation");

      auto block_store = FlatFile::create(store.path);
      if (!block_store) {
        log_->error("Cannot create block store in {}", store.path);
        return nonstd::nullopt;
      }
      log_->info("block store created");

      auto index = std::make_unique<cpp_redis::redis_client>();
      try {
        index->connect(redis.host, redis.port);
      } catch (const cpp_redis::redis_error &e) {
        log_->error(
            "Connection {}:{} with Redis is broken", redis.host, redis.port);
        return nonstd::nullopt;
      }

      log_->info("connection to Redis completed");

      auto postgres_connection =
          std::make_unique<pqxx::lazyconnection>(pg.options());
      try {
        postgres_connection->activate();
      } catch (const pqxx::broken_connection &e) {
        log_->error("Cannot with PostgreSQL is broken: {}", e.what());
        return nonstd::nullopt;
      }
      log_->info("connection to PostgreSQL completed");

      auto wsv_transaction = std::make_unique<pqxx::nontransaction>(
          *postgres_connection, "Storage");
      log_->info("transaction to PostgreSQL initialized");

      return nonstd::make_optional<ConnectionContext>(
          std::move(block_store),
          std::move(index),
          std::move(postgres_connection),
          std::move(wsv_transaction));
    }

    std::shared_ptr<StorageImpl> StorageImpl::create(
        const Config::Redis &redis,
        const Config::Postgres &pg,
        const Config::BlockStorage &store) {
      auto ctx = initConnections(redis, pg, store);
      if (not ctx.has_value()) {
        return nullptr;
      }

      return std::shared_ptr<StorageImpl>(
          new StorageImpl(redis,
                          pg,
                          store,
                          std::move(ctx->block_store),
                          std::move(ctx->index),
                          std::move(ctx->pg_lazy),
                          std::move(ctx->pg_nontx)));
    }

    void StorageImpl::commit(std::unique_ptr<MutableStorage> mutableStorage) {
      std::unique_lock<std::shared_timed_mutex> write(rw_lock_);

      auto storage_ptr = std::move(mutableStorage);  // get ownership of storage
      auto storage = dynamic_cast<MutableStorageImpl *>(storage_ptr.get());
      for (const auto &entry : storage->block_store_) {
        auto &id = entry.first;
        auto &block = entry.second;
        block_store_->add(id,
                          stringToBytes(model::converters::jsonToString(
                              serializer_.serialize(block))));
      }

      storage->index_->exec();
      storage->transaction_->exec("COMMIT;");
      storage->committed = true;
    }

    std::shared_ptr<WsvQuery> StorageImpl::getWsvQuery() const { return wsv_; }

    std::shared_ptr<BlockQuery> StorageImpl::getBlockQuery() const {
      return blocks_;
    }
  }  // namespace ametsuchi
}  // namespace iroha
