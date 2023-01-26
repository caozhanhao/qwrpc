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
#ifndef QWRPC_CLIENT_HPP
#define QWRPC_CLIENT_HPP
#pragma once

#include "method.hpp"
#include "utils.hpp"
#include "libczh/czh.hpp"
#include "httplib.h"
#include <future>

namespace qwrpc::client
{
  class Client
  {
  private:
    std::string addr;
    int port;
    httplib::Client cli;
  public:
    Client(const std::string &addr_, int port_) : addr(addr_), port(port_), cli(addr_, port_) {}
    
    
    template<typename ...Rets, typename ...Args>
    method::MethodRets<Rets...> call(const std::string &method_id, Args &&... args)
    {
      auto internal_args = method::tuple_to_param(std::make_tuple(args...));
      httplib::Params params
          {
              {"id",   method_id},
              {"args", utils::to_str({"args", method::param_to_czh_array(internal_args)})}
          };
      auto res = cli.Get("/qwrpc", params, httplib::Headers{});
      if (res == nullptr)
      {
        auto err = res.error();
        throw error::Error("HTTP error: " + httplib::to_string(err));
      }
      czh::Node node;
      try
      {
        czh::Czh parser(res->body, czh::InputMode::string);
        node = parser.parse();
      }
      catch (czh::error::CzhError &err)
      {
        qwrpc::error::qwrpc_unreachable("Invalid return czh:" + err.get_content());
      }
      catch (czh::error::Error &err)
      {
        qwrpc::error::qwrpc_unreachable("Invalid return czh(libczh internal):" + err.get_content());
      }
      
      error::qwrpc_assert(node.has_node("status") && node["status"].is<std::string>());
      if (node["status"].get<std::string>() != "success")
      {
        error::qwrpc_assert(node.has_node("message") && node["message"].is<std::string>());
        auto err = node["message"].get<std::string>();
        if (err == error::invalid_args)
        {
          error::qwrpc_assert(node.has_node("expected_args") && node["expected_args"].is<czh::value::Array>());
          error::qwrpc_assert(node["expected_args"].get_value().template can_get<std::vector<std::string>>());
          auto expected = node["expected_args"].get<std::vector<std::string>>();
          for (auto &r: expected)
          {
            err += r + ", ";
          }
          if (!expected.empty())
          {
            err.pop_back();
            err.pop_back();
          }
          throw error::Error(err);
        }
        else if (err == error::invalid_args_czh)
        {
          error::qwrpc_assert(node.has_node("czh_error") && node["czh_error"].is<std::string>());
          throw error::Error(err + "[" + node["czh_error"].get<std::string>() + "]");
        }
        else if (err == error::invoke_error)
        {
          error::qwrpc_assert(node.has_node("czh_error") && node["qwrpc_error"].is<std::string>());
          throw error::Error(err + "[" + node["qwrpc_error"].get<std::string>() + "]");
        }
        else
        {
          throw error::Error(err);
        }
      }
      error::qwrpc_assert(node.has_node("return") && node["return"].is<czh::value::Array>());
      auto rets = node["return"].get<czh::value::Array>();
      return method::param_to_tuple<method::TypeList<Rets...>, sizeof...(Rets)>(method::czh_array_to_param(rets));
    }
    
    template<typename ...Rets, typename ...Args>
    auto async_call(const std::string &method_id, Args &&... args)
    {
      return std::async([this](auto &&id, auto &&... args) { return call<Rets...>(id, args...); },
                        method_id, args...);
    }
  };
}
#endif
