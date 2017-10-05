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

#ifndef IROHA_FILESYSTEM_HPP_
#define IROHA_FILESYSTEM_HPP_

#include <boost/assert.hpp>
#include <fstream>
#include <sstream>
#include <string>

namespace iroha {
  namespace filesystem {
    namespace validation {

      /**
       * Possible validators:
       *  given path:
       *    - folder exists
       *    - is file
       *    - is folder
       *    - is symlink
       *    - etc
       *  TODO(@warchant) implement them as separate functions here.
       */

      /**
       * @brief Returns true if file exists by given path, false otherwise.
       * @param path
       * @return
       */
      inline bool is_existing_file(const std::string &path) {
        if (path.empty()) return false;

        // will be autoclosed
        std::ifstream f(path);
        return f.is_open();
      }
    }  // namespace validation

    namespace util {
      /**
       * Open file and read its contents in std::string.
       * @throws std::ios_base::failure if files does not exist.
       * @param path
       * @return
       */
      inline std::string read_file(const std::string &path) {
        // we think that check is performed in caller.
        BOOST_ASSERT(!path.empty());

        std::ifstream f;

        // throw exception if open failed
        f.exceptions(f.exceptions() | std::ifstream::failbit
                     | std::ifstream::badbit);

        // open throws std::ios_base::failure, which is std::system_error (has
        // const
        // char* what() method with the reason)
        f.open(path);

        std::stringstream buffer;
        buffer << f.rdbuf();
        f.close();

        return buffer.str();
      }
    }
  }  // namespace filesystem
}  // namespace iroha

#endif  //  IROHA_FILESYSTEM_HPP_
