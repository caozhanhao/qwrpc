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
#include "qwrpc/qwrpc.hpp"
#include <iostream>

int main()
{
  qwrpc::Client cli("127.0.0.1:8765");
  auto ret = cli.call<int>("add", 1, 1);
  auto[a] = ret;
  std::cout << a << std::endl;
  return 0;
}