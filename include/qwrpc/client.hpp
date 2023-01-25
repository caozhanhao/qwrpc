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

#include "utils.hpp"
#include "libczh/czh.hpp"
#include "httplib.h"

namespace qwrpc::client
{
  class Client
  {
  private:
    std::string addr;
    httplib::Client cli;
  public:
    Client(const std::string &addr_) : addr(addr_), cli(addr_) {}
    
    
    template<utils::MethodRequiredType ...Rets, utils::MethodRequiredType ...Args>
    utils::MethodRets<Rets...> call(const std::string &method_id, Args &&... args)
    {
      auto internal_args = utils::tuple_to_czh_array(std::make_tuple(args...));
      httplib::Params params
          {
              {"id",   method_id},
              {"args", utils::to_str({"args", internal_args})}
          };
      auto res = cli.Get("/qwrpc", params, httplib::Headers{});
      czh::Czh parser(res->body, czh::InputMode::string);
      czh::Node node;
      try
      {
        node = parser.parse();
      }
      catch (czh::error::CzhError &err)
      {//TODO
      }
      catch (czh::error::Error &err)
      {//TODO
      }
      auto rets = node["return"].get<czh::value::Array>();
      return utils::czh_array_to_tuple < utils::TypeList < Rets...>, sizeof...(Rets) > (rets);
    }
  };
}
#endif