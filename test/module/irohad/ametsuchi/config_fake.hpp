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

#ifndef IROHA_CONFIG_MOCK_HPP_
#define IROHA_CONFIG_MOCK_HPP_

#include <gmock/gmock.h>
#include "main/config/config.hpp"

class FakeConfig : public iroha::config::Config {
 public:
  FakeConfig() { load(); }

  void load() override {
    auto pg_host = std::getenv("IROHA_POSTGRES_HOST");
    auto pg_port = std::getenv("IROHA_POSTGRES_PORT");
    auto pg_user = std::getenv("IROHA_POSTGRES_USER");
    auto pg_pass = std::getenv("IROHA_POSTGRES_PASSWORD");
    auto rd_host = std::getenv("IROHA_REDIS_HOST");
    auto rd_port = std::getenv("IROHA_REDIS_PORT");

    if (!pg_host) {
      return;
    }

    this->redis_.host = rd_host;
    this->redis_.port = static_cast<uint16_t>(std::stoull(rd_port));

    this->pg_.host = pg_host;
    this->pg_.port = static_cast<uint16_t>(std::stoull(pg_port));
    this->pg_.username = pg_user;
    this->pg_.password = pg_pass;

    this->db_.path = "/tmp/block_store";
  }
};

#endif  //  IROHA_CONFIG_MOCK_HPP_
