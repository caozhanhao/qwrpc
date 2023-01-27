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
#ifndef QWRPC_RPC_SERVER_HPP
#define QWRPC_RPC_SERVER_HPP
#pragma once

#include "error.hpp"
#include "utils.hpp"
#include "method.hpp"
#include "libczh/czh.hpp"
#include "connector.hpp"
#include <string>
#include <map>
#include <functional>
#include <sstream>
#include <tuple>

namespace qwrpc::error::rpc_server
{
  constexpr auto invalid_request = "Request need to be a valid czh.";
  constexpr auto invalid_argument = "Invalid arguments.";
  constexpr auto invalid_method_id = "Invalid method id.";
  constexpr auto unknown_id = "Unknown method id.";
  constexpr auto invoke_error = "Invoke failed.";
}

namespace qwrpc::rpc_server
{
  class RpcServer
  {
  private:
    std::map<std::string, method::Method> methods;
    int port;
  public:
    RpcServer(int port_) : port(port_) {}
    
    template<typename F>
    RpcServer &register_method(const std::string &name, F &&m)
    {
      methods[name] = method::Method(method::make_method(std::forward<F>(m)));
      return *this;
    }
    
    RpcServer &start()
    {
      connector::Server svr(port, [this](const std::string &request, std::string &res)
      {
        czh::Node req;
        try
        {
          czh::Czh parser(request, czh::InputMode::string);
          req = parser.parse();
        }
        catch (czh::error::CzhError &err)
        {
          res = utils::to_str({{"status",    "failed"},
                               {"message",   error::rpc_server::invalid_request},
                               {"czh_error", err.get_content()}});
          return;
        }
        catch (czh::error::Error &err)
        {
          res = utils::to_str({{"status",    "failed"},
                               {"message",   error::rpc_server::invalid_request},
                               {"czh_error", err.get_content()}});
          return;
        }
        if (!req.has_node("id") && req["id"].is<std::string>())
        {
          res = utils::to_str({{"status",  "failed"},
                               {"message", error::rpc_server::invalid_method_id}});
          return;
        }
        if (!req.has_node("args") && req["args"].is<czh::value::Array>())
        {
          res = utils::to_str({{"status",  "failed"},
                               {"message", error::rpc_server::invalid_argument}});
          return;
        }
        auto id = req["id"].get<std::string>();
        auto args = req["args"].get<czh::value::Array>();
        auto method = methods.find(id);
        if (method == methods.end())
        {
          res = utils::to_str({{"status",  "failed"},
                               {"message", error::rpc_server::unknown_id}});
          return;
        }
        if (!method->second.check_args(args))
        {
          res = utils::to_str({{"status",        "failed"},
                               {"message",       error::rpc_server::invalid_argument},
                               {"expected_args", method->second.expected_args()}});
          return;
        }
        method::MethodParam ret;
        try
        {
          ret = std::move(method->second.call(method::czh_array_to_param(args)));
        }
        catch (error::Error &err)
        {
          res = utils::to_str({{"status",      "failed"},
                               {"message",     error::rpc_server::invoke_error},
                               {"qwrpc_error", err.get_content()}});
          return;
        }
        res = utils::to_str({{"status", "success"},
                             {"return", method::param_to_czh_array(ret)}});
      });
      svr.start();
      return *this;
    }
  };
}
#endif