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
  qwrpc::Client cli("127.0.0.1", 8765);
  auto add_ret = cli.call<int>("add", 1, 1);
  auto[add] = add_ret;
  std::cout << "add: " << add << std::endl;
  auto great_ret = cli.call<qwrpc_example::B>("great_func", qwrpc_example::A{2});
  auto[great] = great_ret;
  std::cout << "great_func: ";
  great.print();
  auto slow_ret = cli.async_call<int>("slow");
  std::cout << "slow called." << std::endl;
  auto[s] = slow_ret.get();
  std::cout << "slow returned " << s << std::endl;
  return 0;
}