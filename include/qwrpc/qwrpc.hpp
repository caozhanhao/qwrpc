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
#ifndef QWRPC_QWRPC_HPP
#define QWRPC_QWRPC_HPP
#pragma once

#include "client.hpp"
#include "error.hpp"
#include "method.hpp"
#include "serializer.hpp"
#include "server.hpp"
#include "utils.hpp"

namespace qwrpc
{
  using server::Server;
  using method::make_method;
  using method::MethodArgs;
  using method::MethodRets;
  using client::Client;
  using serializer::serialize;
  using serializer::deserialize;
}
#endif