//   Copyright 2023 qwrpc - caozhanhao
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
#ifndef QWRPC_SERIALIZER_HPP
#define QWRPC_SERIALIZER_HPP
#pragma once

#include "utils.hpp"
#include "libczh/czh.hpp"
#include <system_error>

namespace qwrpc::serializer
{
  template<typename T>
  std::string serialize(const T &item)
  {
    error::qwrpc_unreachable(
        "Custom Type must define qwrpc::serializer::serialize() and qwrpc::serializer::deserialize()");
    return "";
  }
  
  template<typename T>
  T deserialize(const std::string &str)
  {
    error::qwrpc_unreachable(
        "Custom Type must define qwrpc::serializer::serialize() and qwrpc::serializer::deserialize()");
    return {};
  }
}
#endif