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
#include "example.hpp"
#include "qwrpc/qwrpc.hpp"
#include <iostream>
#include <future>
int main()
{
  qwrpc::RpcClient cli("127.0.0.1", 8765);
  // plus
  auto add_ret = cli.call<int>("plus", 1, 1);
  std::cout << "plus: " << add_ret << std::endl;
  // great_func1
  auto great1_ret = cli.call<qwrpc_example::D>("great_func1", qwrpc_example::C{1});
  std::cout << "great_func1: " << great1_ret.d << std::endl;
  // great_func2
  auto great2_ret = cli.call<qwrpc_example::B>("great_func2", qwrpc_example::A{2});
  std::cout << "great_func2: ";
  great2_ret.print();
  // slow
  auto slow_ret = cli.async_call<int>("slow");
  std::cout << "slow called." << std::endl;
  std::cout << "slow returned " << slow_ret.get() << std::endl;
  return 0;
}