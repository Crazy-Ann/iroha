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

#ifndef IROHA_FLAGS_IMPL_HPP_
#define IROHA_FLAGS_IMPL_HPP_

#include <gflags/gflags.h>
#include <gflags/gflags_completions.h>

#include "logger/logger.hpp"
#include "util/filesystem.hpp"
#include "util/network.hpp"
#include "util/string.hpp"

using iroha::string::validation::is_printable;
using iroha::network::validation::is_host_valid;
using iroha::network::validation::is_port_valid;
using iroha::filesystem::validation::is_existing_file;

static auto log_ = logger::log("config");

bool host_validator(const char *flagname, const std::string &host) {
  if (!is_host_valid(host)) {
    log_->warn("-{} {} is invalid", flagname, host);
    return false;
  }

  return true;
}

bool port_validator(const char *flagname, int32_t port) {
  if (!is_port_valid<int32_t>(port)) {
    log_->warn("-{} {} is invalid", flagname, port);
    return false;
  }

  return true;
}

bool non_empty_validator(const char *flagname, const std::string &s) {
  return !s.empty();
}

bool filepath_validator(const char *flagname, const std::string &path) {
  if (path.empty()) {
    return false;
  }

  if (is_existing_file(path)) {
    log_->warn("-{} {} does not exist", flagname, path);
    return false;
  }

  return true;
}

/// -new_ledger
DEFINE_bool(create, false, "Create new ledger with given genesis block");

DEFINE_string(genesis_block, "genesis.json", "Define path to genesis block");
DEFINE_validator(genesis_block,
                 [](const char *flagname, const std::string &path) {
                   // it is necessary to set flag create=true to use genesis
                   // block
                   if (FLAGS_create) {
                     return filepath_validator(flagname, path);
                   }

                   return true;
                 });

/// Redis
// redis host
DEFINE_string(redis_host,
              "localhost",
              "Define host on which redis is listening");
DEFINE_validator(redis_host, &host_validator);

// redis port
DEFINE_int32(redis_port, 6379, "Define port on which redis is listening");
DEFINE_validator(redis_port, &port_validator);

/// PostgreSQL host and port, username and password
// postgres host
DEFINE_string(postgres_host,
              "localhost",
              "Define host on which postgres is listening");
DEFINE_validator(postgres_host, &host_validator);

// postgres port
DEFINE_int32(postgres_port,
             5432,
             "Define port on which postgresql is listening");
DEFINE_validator(postgres_port, &port_validator);

// postgres username
DEFINE_string(postgres_username, "postgres", "Define postgres user");
DEFINE_validator(postgres_username, &non_empty_validator);

// postgres password
DEFINE_string(postgres_password, "", "Define password for postgres user");
DEFINE_validator(postgres_password, &non_empty_validator);

/// Keypair
// certificate
DEFINE_string(certificate, "", "Define path to certificate in PEM format");
DEFINE_validator(certificate, &filepath_validator);

// key
DEFINE_string(key, "", "Define path to EC private key in PEM format");
DEFINE_validator(key, &filepath_validator);

/// BlockStorage
// database path
DEFINE_string(dbpath, "", "Define path to folder for block storage");
DEFINE_validator(dbpath, &non_empty_validator);

#endif  //  IROHA_FLAGS_IMPL_HPP_
