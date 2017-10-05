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

#include "gflags_config.hpp"
#include <sstream>
#include "main/config/flags.hpp"

namespace iroha {
  namespace config {

    GFlagsConfig::GFlagsConfig() { load(); }

    void GFlagsConfig::load() {
      gflags::ReparseCommandLineNonHelpFlags();

      // these parameters are already validated.
      this->redis_.host = FLAGS_redis_host;
      this->redis_.port = static_cast<uint16_t>(FLAGS_redis_port);

      this->pg_.host = FLAGS_postgres_host;
      this->pg_.port = static_cast<uint16_t>(FLAGS_postgres_port);
      this->pg_.username = FLAGS_postgres_username;
      this->pg_.password = FLAGS_postgres_password;

      this->crypto_.certificate = FLAGS_certificate;
      this->crypto_.key = FLAGS_key;

      this->db_.path = FLAGS_dbpath;

      this->options_.genesis_block = FLAGS_genesis_block;

      this->loaded_ = true;
    }
  }  // namespace config
}  // namespace iroha
