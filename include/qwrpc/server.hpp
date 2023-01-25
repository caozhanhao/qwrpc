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
#include "libczh/czh.hpp"
#include "httplib.h"
#include <string>
#include <map>
#include <functional>
#include <sstream>
#include <tuple>

namespace qwrpc::server
{
  template<typename T>
  struct method_helper;
  template<typename Ret, typename T, typename... Args>
  struct method_helper<Ret(T::*)(Args...) const>
  {
    using type = std::function<Ret(Args...)>;
  };
  
  template<typename F>
  typename method_helper<decltype(&F::operator())>::type
  method(F const &m)
  {
    return m;
  }
  
  class Method
  {
  private:
    std::function<czh::value::Array(czh::value::Array)> func;
    std::vector<size_t> args;
  public:
    Method() = default;
    
    Method(std::function<czh::value::Array(czh::value::Array)> f, std::vector<size_t> args_)
        : func(std::move(f)), args(args_) {}
    
    bool check_args(const czh::value::Array &call_args) const
    {
      if (call_args.size() != args.size()) return false;
      for (size_t i = 0; i < args.size(); ++i)
      {
        if (args[i] != call_args[i].index())
        {
          return false;
        }
      }
      return true;
    }
    
    czh::value::Array call(const czh::value::Array &call_args) const
    {
      return func(call_args);
    }
    
    czh::value::Array expected_args() const
    {
      czh::value::Array ret;
      for (auto &r: args)
      {
        ret.emplace_back(czh::value::details::get_typename(r));
      }
      return ret;
    }
  };
  
  class Server
  {
  private:
    std::map<std::string, Method> methods;
  public:
    template<utils::MethodRequiredType ...Args, utils::MethodRequiredType ...Rets>
    Server &register_method(const std::string &name,
                            std::function<utils::MethodRets<Rets...>(utils::MethodArgs<Args...>)> m)
    {
      auto packed = [m](const czh::value::Array &args) -> czh::value::Array
      {
        auto internal_args = utils::czh_array_to_tuple<utils::TypeList<Args...>, sizeof...(Args)>(args);
        utils::MethodRets<Rets...> internal_ret = m(internal_args);
        czh::value::Array ret = utils::tuple_to_czh_array(internal_ret);
        return ret;
      };
      auto args = utils::tuple_to_czh_type_index(utils::MethodArgs<Args...>{});
      methods[name] = Method(std::move(packed), std::move(args));
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
                                         {"message", "Need id."}}), "text/plain");
          return;
        }
        if (!req.has_param("args"))
        {
          res.set_content(utils::to_str({{"status",  "failed"},
                                         {"message", "Need arguments."}}), "text/plain");
          return;
        }
        auto id = req.get_param_value("id");
        auto argstr = req.get_param_value("args");
        czh::Czh parser(argstr, czh::InputMode::string);
        czh::Node node;
        try
        {
          node = parser.parse();
        }
        catch (czh::error::CzhError &err)
        {
          res.set_content(utils::to_str({{"status",    "failed"},
                                         {"message",   "Argument is not a valid czh."},
                                         {"czh_error", err.get_content()}}), "text/plain");
          return;
        }
        catch (czh::error::Error &err)
        {
          res.set_content(utils::to_str({{"status",    "failed"},
                                         {"message",   "Argument is not a valid czh.(libczh internal)"},
                                         {"czh_error", err.get_content()}}), "text/plain");
          return;
        }
        
        if (!node.has_node("args"))
        {
          res.set_content(utils::to_str({{"status",  "failed"},
                                         {"message", "Arguments must be named 'args'."}}), "text/plain");
          return;
        }
        if (!node["args"].is<czh::value::Array>())
        {
          res.set_content(utils::to_str({{"status",  "failed"},
                                         {"message", "Argument must be a Array."}}), "text/plain");
          return;
        }
        auto args = node["args"].get<czh::value::Array>();
        auto method = methods.find(id);
        if (method == methods.end())
        {
          res.set_content(utils::to_str({{"status",  "failed"},
                                         {"message", "Unknown method id."}}), "text/plain");
          return;
        }
        if (!method->second.check_args(args))
        {
          res.set_content(utils::to_str({{"status",        "failed"},
                                         {"message",       "Invalid argument."},
                                         {"expected_args", method->second.expected_args()}}), "text/plain");
          return;
        }
        auto ret = method->second.call(args);
        res.set_content(utils::to_str({{"status", "success"},
                                       {"return", ret}}), "text/plain");
      });
      svr.listen("localhost", 8765);
      return *this;
    }
  };
}
#endif