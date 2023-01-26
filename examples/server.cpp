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
int main()
{
  qwrpc::Server svr;
  svr.register_method("add",
                      [](qwrpc::MethodArgs<int, int> args)
                          -> qwrpc::MethodRets<int>
                      {
                        auto[rhs, lhs] = args;
                        return {rhs + lhs};
                      });
  svr.register_method("great_func",
                      [](qwrpc::MethodArgs<qwrpc_example::A> args)
                          -> qwrpc::MethodRets<qwrpc_example::B>
                      {
                        auto[a] = args;
                        return qwrpc_example::B{std::to_string(a.get_data() + 1)};
                      });
  svr.start();
  return 0;
}