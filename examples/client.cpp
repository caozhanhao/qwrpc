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
  // foo1
  auto foo1_ret = cli.call<qwrpc_example::D>("foo1", qwrpc_example::C{1});
  std::cout << "foo1: " << foo1_ret.d << std::endl;
  // foo2
  auto foo2_ret = cli.call<std::vector<std::vector<qwrpc_example::C>>>("foo2",
                                                                       std::vector<qwrpc_example::C>{
                                                                           qwrpc_example::C{1}});
  std::cout << "foo2: ";
  for (auto &r: foo2_ret[0]) std::cout << r.c << " ";
  std::cout << std::endl;
  // foo3
  auto foo3_ret = cli.call<std::vector<qwrpc_example::B>>("foo3", qwrpc_example::A{2});
  std::cout << "foo3: ";
  foo3_ret[0].print();
  // slow
  auto slow_ret = cli.async_call<std::string>("slow", std::string(""));
  std::cout << "slow called." << std::endl;
  std::cout << "slow returned: " << slow_ret.get() << std::endl;
  return 0;
}