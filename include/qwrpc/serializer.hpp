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
#include <vector>
#include <string>

namespace qwrpc::serializer
{
  template<typename T>
  T deserialize(const std::string &str);
  
  template<typename T>
  std::string serialize(const T &item);
  
  namespace details
  {
    template<typename T>
    constexpr bool is_serializable_single_type_v = std::is_trivially_copyable_v<T> || std::is_same_v<T, std::string>;
    
    template<typename T>
    struct is_serializable;
    
    template<typename BeginIt, typename EndIt>
    concept SerializableItRange =
    requires(BeginIt begin_it, EndIt end_it)
    {
      { ++begin_it };
      { *begin_it };
      requires !std::is_void_v<decltype(*begin_it)>;
      // There might be specialization of serialize/deserialize, and it can not detect it.
      // So don't enable it.
      // requires(is_serializable<std::remove_cvref_t<decltype(*begin_it)>>::value);
      { begin_it != end_it };
    };
    
    template<typename T>
    concept SerializableContainer =
    requires(T value)
    {
      { std::begin(value) };
      { std::end(value) };
      requires SerializableItRange<decltype(std::begin(value)), decltype(std::end(
          value))>;
      { value.insert(std::end(value), std::declval<decltype(*std::begin(value))>()) };
    };
    
    template<typename T, typename U = void>
    struct is_serializable_container : public std::false_type {};
    template<typename T> requires SerializableContainer<T>
    struct is_serializable_container<T> : public std::true_type {};
    
    template<typename T>
    constexpr bool is_serializable_container_v = is_serializable_container<T>::value;
    
    template<typename T>
    struct is_serializable
    {
      static constexpr bool value = (is_serializable_container_v<T> || is_serializable_single_type_v<T>);
    };
    
    template<typename T>
    constexpr bool is_serializable_v = is_serializable<T>::value;
    
    template<typename T>
    concept Serializable = is_serializable_v<T>;
    struct NotImplemented {};
    struct TriviallyCopyable {};
    struct Container {};
    struct StdString {};
    template<Serializable T>
    struct TagDispatch
    {
      using tag = std::conditional_t<std::is_trivially_copyable_v<std::decay_t<T>>, TriviallyCopyable,
          std::conditional_t<std::is_same_v<std::decay_t<T>, std::string>, StdString,
              std::conditional_t<is_serializable_container_v<std::decay_t<T>>, Container,
                  NotImplemented>>>;
    };
    
    template<typename T>
    std::string internal_serialize(NotImplemented, const T &item) = delete;
    
    template<typename T>
    T internal_deserialize(NotImplemented, const std::string &str) = delete;
    
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
      error::qwrpc_assert(str.size() == sizeof(T));
      T item;
      std::memcpy(&item, str.data(), sizeof(T));
      return item;
    }
    
    template<typename T>
    std::string internal_serialize(StdString, const T &item)
    {
      return item;
    }
    
    template<typename T>
    T internal_deserialize(StdString, const std::string &str)
    {
      return str;
    }
    
    template<typename T>
    std::string internal_serialize(Container, const T &item)
    {
      using value_type = std::remove_cvref_t<decltype(*std::begin(std::declval<T>()))>;
      czh::Node ret;
      size_t i = 0;
      for (auto &r: item)
      {
        ret.add_node("value" + std::to_string(i));
        ret["value" + std::to_string(i)].add("v",
                                             serialize<value_type>(
                                                 static_cast<value_type>(const_cast<std::remove_const_t<decltype(r)>>(r))));
        ++i;
      }
      return utils::to_str(ret);
    }
    
    template<typename T>
    T internal_deserialize(Container, const std::string &str)
    {
      using value_type = std::remove_cvref_t<decltype(*std::begin(std::declval<T>()))>;
      czh::Czh p(str, czh::InputMode::string);
      T ret;
      auto node = p.parse();
      for (auto &r: node)
      {
        ret.insert(std::end(ret), deserialize<value_type>(r["v"].get<std::string>()));
      }
      return ret;
    }
  }
  
  template<typename T>
  std::string serialize(const T &item)
  {
    static_assert(!std::is_same_v<typename details::TagDispatch<T>::tag, details::NotImplemented>,
                  "Custom Type must define qwrpc::serializer::serialize() and qwrpc::serializer::deserialize()");
    return details::internal_serialize<T>(typename details::TagDispatch<T>::tag{}, item);
  }
  
  template<typename T>
  T deserialize(const std::string &str)
  {
    static_assert(!std::is_same_v<typename details::TagDispatch<T>::tag, details::NotImplemented>,
                  "Custom Type must define qwrpc::serializer::serialize() and qwrpc::serializer::deserialize()");
    return details::internal_deserialize<T>(typename details::TagDispatch<T>::tag{}, str);
  }
}
#endif