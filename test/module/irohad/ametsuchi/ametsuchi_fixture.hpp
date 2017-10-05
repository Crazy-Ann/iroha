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

#ifndef IROHA_AMETSUCHI_FIXTURE_HPP
#define IROHA_AMETSUCHI_FIXTURE_HPP

#include "common/files.hpp"
#include "logger/logger.hpp"
#include "main/config/impl/gflags_config.hpp"

#include <gtest/gtest.h>
#include <cpp_redis/cpp_redis>
#include <pqxx/pqxx>
#include "config_fake.hpp"

namespace iroha {
  namespace ametsuchi {


    /**
     * Class with ametsuchi initialization
     */
    class AmetsuchiTest : public ::testing::Test {
     protected:
      virtual void SetUp() {
        auto log = logger::testLog("AmetsuchiTest");

        config_ = std::make_unique<FakeConfig>();
        mkdir(config_->blockStorage().path.c_str(),
              S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      }
      virtual void TearDown() {
        const auto drop = R"(
DROP TABLE IF EXISTS account_has_signatory;
DROP TABLE IF EXISTS account_has_asset;
DROP TABLE IF EXISTS role_has_permissions;
DROP TABLE IF EXISTS account_has_roles;
DROP TABLE IF EXISTS account_has_grantable_permissions;
DROP TABLE IF EXISTS account;
DROP TABLE IF EXISTS asset;
DROP TABLE IF EXISTS domain;
DROP TABLE IF EXISTS signatory;
DROP TABLE IF EXISTS peer;
DROP TABLE IF EXISTS role;
)";

        pqxx::connection connection(config_->postgres().options());
        pqxx::work txn(connection);
        txn.exec(drop);
        txn.commit();
        connection.disconnect();

        cpp_redis::redis_client client;
        client.connect(config_->redis().host, config_->redis().port);
        client.flushall();
        client.sync_commit();
        client.disconnect();

        iroha::remove_all(config_->blockStorage().path);
      }

      std::unique_ptr<iroha::config::Config> config_;

      //      std::string pgopt_ =
      //          "host=localhost port=5432 user=postgres
      //          password=mysecretpassword";
      //
      //      std::string redishost_ = "localhost";
      //      size_t redisport_ = 6379;
      //
      //      std::string block_store_path = "/tmp/block_store";
    };
  }  // namespace ametsuchi
}  // namespace iroha

#endif  // IROHA_AMETSUCHI_FIXTURE_HPP
