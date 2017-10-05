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

#ifndef IROHA_VALIDATORIMPL_HPP_
#define IROHA_VALIDATORIMPL_HPP_

#include "util/network.hpp"
#include "util/validator.hpp"

namespace iroha {

  /**
   * @class ValidatorImpl
   */
  class ValidatorImpl final : public Validator {
    ///
    bool isPortValid(int port) const noexcept override;

    ///
    bool isHostValid(const std::string &host) const noexcept override;
  };
}

#endif  //  IROHA_VALIDATORIMPL_HPP_
