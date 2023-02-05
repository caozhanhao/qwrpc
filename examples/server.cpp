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
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

int main()
{
  qwrpc::RpcServer svr(8765);
  // Some functions can be used directly
  svr.register_method("plus", std::plus<int>());
  // or svr.register_method("plus", [](int a, int b){return a + b;});
  
  // Trivially copyable type don't need to specialize qwrpc::serializer::...
  // in example.hpp:
  //  struct C { int c; };
  //  struct D { int d; };
  svr.register_method("foo1",
                      [](qwrpc_example::C c) -> qwrpc_example::D
                      {
                        return {c.c + 1};
                      });
  // All containers that store serializer type also don't need it.
  svr.register_method("foo2",
                      [](std::vector<qwrpc_example::C> c) -> std::vector<std::vector<qwrpc_example::C>>
                      {
                        c.emplace_back(qwrpc_example::C{6});
                        return {c};
                      });
  // Other type need the specialization, see example.hpp.
  svr.register_method("foo3",
                      [](qwrpc_example::A a) -> std::vector<qwrpc_example::B>
                      {
                        return {qwrpc_example::B{std::to_string(a.get_data() + 1)}};
                      });
  // async
  svr.register_method("slow",
                      [](std::string a) -> std::string
                      {
                        std::this_thread::sleep_for(10s);
                        return a + " 10 seconds later";
                      });
  svr.start();
  return 0;
}