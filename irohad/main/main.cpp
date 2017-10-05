/*
Copyright Soramitsu Co., Ltd. 2016 All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "util/filesystem.hpp"

#include <gflags/gflags.h>
#include <gflags/gflags_completions.h>
#include "main/config/impl/gflags_config.hpp"
#include "main/raw_block_insertion.hpp"
#include "main/service.hpp"

#include "logger/logger.hpp"

using iroha::config::Config;
using iroha::config::GFlagsConfig;
using Iroha = Service;
using iroha::filesystem::util::read_file;

int main(int argc, char *argv[]) {
  auto log = logger::log("MAIN");

  try {
#ifdef IROHA_VERSION
// preprocessor trick, which helps to stringify arbitrary value
#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)
    gflags::SetVersionString(STRINGIFY(IROHA_VERSION));
#endif
    std::stringstream ss;
    ss << "\n"
       << "\n"
       << "USING ENVIRONMENT VARIABLES: \n"
       << "\t$ export FLAGS_flag1=val\n"
       << "\t$ export FLAGS_flag2=val\n"
       << "\t$ " << argv[0] << " -fromenv=flag1,flag2\n"
       << "\n"
       << "USING CLI: \n"
       << "\t$ " << argv[0] << " -flag1=val -flag2=val\n"
       << "\n"
       << "USING FLAG FILE: \n"
       << "\t$ cat /tmp/flags\n"
       << "\t-flag1=val\n"
       << "\t-flag2=val\n"
       << "\t$ " << argv[0] << " -flagfile=/tmp/flags\n";

    gflags::SetUsageMessage(ss.str());
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::HandleCommandLineCompletions();

    // if something critical can not be parsed, throws exceptions descendants
    // from std::exception
    Iroha irohad(std::make_unique<GFlagsConfig>());

    // TODO(@warchant): refactor. Move this to Iroha as a separate
    // module

    {  // bad :(
      iroha::main::BlockInserter inserter(irohad.storage);

      // throws if can not open file
      auto content = read_file(irohad.config().options().genesis_block);
      auto block = inserter.parseBlock(content);
      log->info("Block parsed");

      if (block.has_value()) {
        inserter.applyToLedger({block.value()});
        log->info("Genesis block inserted, number of transactions: {}",
                  block.value().transactions.size());
      } else {
        throw std::logic_error("Block can not be parsed from JSON");
      }
    }

    // init pipeline components
    log->info("start initialization");
    irohad.init();

    // runs iroha
    log->info("iroha initialized");
    irohad.run();

  } catch (const std::exception &e) {
    log->error("FATAL: {}", e.what());
  }

  // cleanup
  gflags::ShutDownCommandLineFlags();

  return 0;
}
