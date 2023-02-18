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

#include "logger.hpp"
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
  constexpr auto invalid_request = "Request needs to be a valid czh.";
  constexpr auto invalid_argument = "Invalid arguments.";
  constexpr auto invalid_expected_ret = "Invalid expected_ret.";
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
      methods[name] = method::Method(std::function(std::forward<F>(m)));
      logger::info(logger::no_fmt, "Method Register: ", name);
      return *this;
    }
    
    RpcServer &start()
    {
      connector::Server svr(port, [this](const connector::Req &request, connector::Res &res)
      {
        logger::info(logger::no_fmt,
                     "Received request from: ", request.get_ip(), ", request: ", request.get_content());
        czh::Node req;
        try
        {
          czh::Czh parser(request.get_content(), czh::InputMode::string);
          req = parser.parse();
        }
        catch (czh::error::CzhError &err)
        {
          res.set_content(utils::to_str({{"status",    "failed"},
                                         {"message",   error::rpc_server::invalid_request},
                                         {"czh_error", err.get_content()}}));
          logger::warn(logger::no_fmt,
                       "Respond to: ", request.get_ip(),
                       ", response: ", res.get_content());
          return;
        }
        catch (czh::error::Error &err)
        {
          res.set_content(utils::to_str({{"status",    "failed"},
                                         {"message",   error::rpc_server::invalid_request},
                                         {"czh_error", err.get_content()}}));
          logger::warn(logger::no_fmt,
                       "Respond to: ", request.get_ip(),
                       ", response: ", res.get_content());
          return;
        }
        if (!req.has_node("id") && req["id"].is<std::string>())
        {
          res.set_content(utils::to_str({{"status",  "failed"},
                                         {"message", error::rpc_server::invalid_method_id}}));
          logger::warn(logger::no_fmt,
                       "Respond to: ", request.get_ip(),
                       ", response: ", res.get_content());
          return;
        }
        if (!req.has_node("args") && req["expected_ret"].is<std::string>())
        {
          res.set_content(utils::to_str({{"status",  "failed"},
                                         {"message", error::rpc_server::invalid_expected_ret}}));
          logger::warn(logger::no_fmt,
                       "Respond to: ", request.get_ip(),
                       ", response: ", res.get_content());
          return;
        }
        if (!req.has_node("args") && req["args"].is<czh::value::Array>())
        {
          res.set_content(utils::to_str({{"status",  "failed"},
                                         {"message", error::rpc_server::invalid_argument}}));
          logger::warn(logger::no_fmt,
                       "Respond to: ", request.get_ip(),
                       ", response: ", res.get_content());
          return;
        }
        auto id = req["id"].get<std::string>();
        auto args = req["args"].get<czh::value::Array>();
        auto method = methods.find(id);
        if (method == methods.end())
        {
          res.set_content(utils::to_str({{"status",  "failed"},
                                         {"message", error::rpc_server::unknown_id}}));
          logger::warn(logger::no_fmt,
                       "Respond to: ", request.get_ip(),
                       ", response: ", res.get_content());
          return;
        }
        if (!method->second.check_args(args))
        {
          res.set_content(utils::to_str({{"status",        "failed"},
                                         {"message",       error::rpc_server::invalid_argument},
                                         {"expected_args", method->second.expected_args()}}));
          logger::warn(logger::no_fmt,
                       "Respond to: ", request.get_ip(),
                       ", response: ", res.get_content());
          return;
        }
        if (!method->second.check_ret(req["expected_ret"].get<std::string>()))
        {
          res.set_content(utils::to_str({{"status",       "failed"},
                                         {"message",      error::rpc_server::invalid_expected_ret},
                                         {"expected_ret", method->second.expected_ret()}}));
          logger::warn(logger::no_fmt,
                       "Respond to: ", request.get_ip(),
                       ", response: ", res.get_content());
          return;
        }
        method::MethodParam ret;
        try
        {
          ret = std::move(method->second.call(args));
        }
        catch (error::Error &err)
        {
          res.set_content(utils::to_str({{"status",      "failed"},
                                         {"message",     error::rpc_server::invoke_error},
                                         {"qwrpc_error", err.get_content()}}));
          logger::warn(logger::no_fmt,
                       "Respond to: ", request.get_ip(),
                       ", response: ", res.get_content());
          return;
        }
        res.set_content(utils::to_str({{"status", "success"},
                                       {"return", method::ret_to_czh_type(ret)}}));
        logger::info(logger::no_fmt,
                     "Respond to: ", request.get_ip(),
                     ", response: ", res.get_content());
      });
      svr.start();
      return *this;
    }
  };
}
#endif