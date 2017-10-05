/**
 * @copyright Soramitsu Co., Ltd. 2017 All Rights Reserved.
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

#ifndef IROHA_VALIDATOR_HPP_
#define IROHA_VALIDATOR_HPP_

namespace iroha {
  /**
   * @class Validator
   *
   * Used to validate any kind of information, such as network, fs or string
   * constraints.
   */
  class Validator {
   public:
    /**
     * Checks if port is valid.
     *
     * Performed checks:
     *  - port is in given range: 1 < port < 65535.
     *
     * @tparam T comparable with int type (>, <).
     * @return
     */
    virtual bool isPortValid(int port) const noexcept = 0;

    /**
     * Checks if host is valid. Because of check complexity, it is hard to say
     * if a string looks like a domain or IP.
     *
     * Performed checks:
     *  - string contains only printable symbols
     *  - string is non-empty
     *
     * @param host
     * @return
     */
    virtual bool isHostValid(const std::string &host) const noexcept = 0;
  };
}

#endif  //  IROHA_VALIDATOR_HPP_
