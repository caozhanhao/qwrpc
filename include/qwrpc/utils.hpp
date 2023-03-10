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
#ifndef QWRPC_UTILS_HPP
#define QWRPC_UTILS_HPP
#pragma once

#include "libczh/czh.hpp"
#include <functional>
#include <sstream>
#include <tuple>

namespace qwrpc::utils
{
  std::string to_str(const czh::Node &n)
  {
    std::stringstream ss;
    czh::BasicWriter<std::stringstream> bw{ss};
    n.accept(bw);
    return ss.str();
  }
}
#endif