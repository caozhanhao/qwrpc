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
#ifndef QWRPC_SERVER_HPP
#define QWRPC_SERVER_HPP
#pragma once

#include "error.hpp"
#include "utils.hpp"
#include "method.hpp"
#include "libczh/czh.hpp"
#include "httplib.h"
#include <string>
#include <map>
#include <functional>
#include <sstream>
#include <tuple>

namespace qwrpc::server
{
  class Server
  {
  private:
    std::map<std::string, method::Method> methods;
    int port;
  public:
    Server(int port_) : port(port_) {}
  
    template<typename F>
    Server &register_method(const std::string &name, F &&m)
    {
      methods[name] = method::Method(method::make_method(std::forward<F>(m)));
      return *this;
    }
  
    Server &start()
    {
      httplib::Server svr;
      svr.Get("/qwrpc", [this](const httplib::Request &req, httplib::Response &res)
      {
        if (!req.has_param("id"))
        {
          res.set_content(utils::to_str({{"status",  "failed"},
                                         {"message", error::no_method_id}}), "text/plain");
          res.status = 500;
          return;
        }
        if (!req.has_param("args"))
        {
          res.set_content(utils::to_str({{"status",  "failed"},
                                         {"message", error::no_args}}), "text/plain");
          res.status = 500;
          return;
        }
        auto id = req.get_param_value("id");
        auto argstr = req.get_param_value("args");
        czh::Node node;
        try
        {
          czh::Czh parser(argstr, czh::InputMode::string);
          node = parser.parse();
        }
        catch (czh::error::CzhError &err)
        {
          res.set_content(utils::to_str({{"status",    "failed"},
                                         {"message",   error::invalid_args_czh},
                                         {"czh_error", err.get_content()}}), "text/plain");
          res.status = 500;
          return;
        }
        catch (czh::error::Error &err)
        {
          res.set_content(utils::to_str({{"status",    "failed"},
                                         {"message",   error::invalid_args_czh},
                                         {"czh_error", err.get_content()}}), "text/plain");
          res.status = 500;
          return;
        }
        
        if (!node.has_node("args"))
        {
          res.set_content(utils::to_str({{"status",  "failed"},
                                         {"message", error::invalid_args_name}}), "text/plain");
          res.status = 500;
          return;
        }
        if (!node["args"].is<czh::value::Array>())
        {
          res.set_content(utils::to_str({{"status",  "failed"},
                                         {"message", error::invalid_args_czhtype}}), "text/plain");
          res.status = 500;
          return;
        }
        auto args = node["args"].get<czh::value::Array>();
        auto method = methods.find(id);
        if (method == methods.end())
        {
          res.set_content(utils::to_str({{"status",  "failed"},
                                         {"message", error::unknown_id}}), "text/plain");
          res.status = 500;
          return;
        }
        if (!method->second.check_args(args))
        {
          res.set_content(utils::to_str({{"status",        "failed"},
                                         {"message",       error::invalid_args},
                                         {"expected_args", method->second.expected_args()}}), "text/plain");
          res.status = 500;
          return;
        }
        method::MethodParam ret;
        try
        {
          ret = std::move(method->second.call(method::czh_array_to_param(args)));
        }
        catch (error::Error &err)
        {
          res.set_content(utils::to_str({{"status",      "failed"},
                                         {"message",     error::invoke_error},
                                         {"qwrpc_error", err.get_content()}}), "text/plain");
          res.status = 500;
          return;
        }
        res.set_content(utils::to_str({{"status", "success"},
                                       {"return", method::param_to_czh_array(ret)}}), "text/plain");
        res.status = 200;
      });
      svr.set_exception_handler(
          [](const auto &req, auto &res, std::exception_ptr ep)
          {
            res.set_content(utils::to_str({{"status",  "failed"},
                                           {"message", error::unknown_error}}), "text/plain");
            res.status = 500;
          });
      svr.listen("localhost", port);
      return *this;
    }
  };
}
#endif