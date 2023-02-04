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
#ifndef QWRPC_SERIALIZER_HPP
#define QWRPC_SERIALIZER_HPP
#pragma once

#include "utils.hpp"
#include "libczh/czh.hpp"
#include <type_traits>

namespace qwrpc::serializer
{
  namespace detail
  {
    struct NotImplemented {};
    struct TriviallyCopyable {};
    template<typename T>
    struct TagDispatch
    {
      using tag = std::conditional_t<std::is_trivially_copyable_v<std::decay_t<T>>, TriviallyCopyable, NotImplemented>;
    };
    
    template<typename T>
    std::string internal_serialize(NotImplemented, const T &item)
    {
      error::qwrpc_not_implemented(
          "Custom Type must define qwrpc::serializer::serialize() and qwrpc::serializer::deserialize()");
      return "";
    }
    
    template<typename T>
    T internal_deserialize(NotImplemented, const std::string &str)
    {
      error::qwrpc_not_implemented(
          "Custom Type must define qwrpc::serializer::serialize() and qwrpc::serializer::deserialize()");
      return {};
    }
    
    template<typename T>
    std::string internal_serialize(TriviallyCopyable, const T &item)
    {
      std::vector<char> ret(sizeof(T));
      std::memcpy(ret.data(), &item, sizeof(T));
      std::string str;
      for (auto &r: ret) { str += r; }
      return str;
    }
    
    template<typename T>
    T internal_deserialize(TriviallyCopyable, const std::string &str)
    {
      T item;
      std::memcpy(&item, str.data(), sizeof(T));
      return item;
    }
  }
  
  template<typename T>
  std::string serialize(const T &item)
  {
    return detail::internal_serialize<T>(typename detail::TagDispatch<T>::tag{}, item);
  }
  
  template<typename T>
  T deserialize(const std::string &str)
  {
    return detail::internal_deserialize<T>(typename detail::TagDispatch<T>::tag{}, str);
  }
}
#endif