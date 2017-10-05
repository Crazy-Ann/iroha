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

#ifndef IROHA_NETWORK_HPP_
#define IROHA_NETWORK_HPP_

#include <cstdint>
#include <string>
#include "string.hpp"

namespace iroha {
    namespace network {
      namespace validation {

      /**
       * @tparam T integer type
       * @return true if 2^16 > port > 0, false otherwise.
       */
      template<typename T>
      constexpr inline bool is_port_valid(T port) {
        return std::numeric_limits<T>::is_integer && port > 0
            && port < 65535 /* 1 << 16 - 1 */;
      }

      /**
       * It is not possible to determine all possible types of hostnames, since
       * they may be localized (domain .рф in Russia is example).
       *
       * @return true if given string looks like a domain name or ip.
       */
      inline bool is_host_valid(const std::string &host) {
        return !host.empty() && string::validation::is_printable(host);
      }

    } // namespace validation
  } // namespace network
} // namespace iroha

#endif  //  IROHA_NETWORK_HPP_
