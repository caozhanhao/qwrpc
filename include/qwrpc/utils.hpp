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
#ifndef QWRPC_UTILS_HPP
#define QWRPC_UTILS_HPP
#pragma once

#include "libczh/czh.hpp"
#include <functional>
#include <sstream>
#include <tuple>

namespace qwrpc::utils
{
  std::string to_str(const czh::Node &n)
  {
    std::stringstream ss;
    czh::BasicWriter<std::stringstream> bw{ss};
    n.accept(bw);
    return ss.str();
  }
  
  using czh::value::details::TypeList;
  using czh::value::details::index_at_t;
  using czh::value::details::index_of_v;
  template<typename T>
  concept MethodRequiredType =  czh::value::details::is_czh_basic_type_v<T>;
  template<MethodRequiredType ...Args>
  using MethodArgs = std::tuple<Args...>;
  template<MethodRequiredType ...Rets>
  using MethodRets = std::tuple<Rets...>;
  
  // adapted from: https://stackoverflow.com/questions/28410697/c-convert-vector-to-tuple
  template<typename List, std::size_t... index>
  auto czh_array_to_tuple_helper(const czh::value::Array &v, std::index_sequence<index...>)
  {
    return std::make_tuple(std::get<index_at_t<index, List>>(v[index])...);
  }
  
  template<typename List, std::size_t N>
  auto czh_array_to_tuple(const czh::value::Array &v)
  {
    return czh_array_to_tuple_helper<List>(v, std::make_index_sequence<N>());
  }
  
  // adapted from: https://stackoverflow.com/questions/42494715/c-transform-a-stdtuplea-a-a-to-a-stdvector-or-stddeque
  template<typename Tuple>
  czh::value::Array tuple_to_czh_array(Tuple &&tuple)
  {
    return std::apply([](auto &&... elems)
                      {
                        return czh::value::Array{std::forward<decltype(elems)>(elems)...};
                      }, std::forward<Tuple>(tuple));
  }
  
  template<typename Tuple>
  std::vector<size_t> tuple_to_czh_type_index(Tuple &&tuple)
  {
    return std::apply([](auto &&... elems)
                      {
                        return std::vector<size_t>{
                            static_cast<size_t>(index_of_v<std::decay_t<decltype(elems)>, czh::value::details::BasicVTList>)...};
                      }, std::forward<Tuple>(tuple));
  }
}
#endif