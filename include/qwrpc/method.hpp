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
#ifndef QWRPC_METHOD_HPP
#define QWRPC_METHOD_HPP
#pragma once

#include "error.hpp"
#include "serializer.hpp"
#include "libczh/czh.hpp"
#include <vector>
#include <tuple>
#include <functional>
#include <variant>
#include <algorithm>

namespace qwrpc::method
{
  template<typename... List>
  struct TypeList {};
  
  struct TypeListError {};
  
  template<typename... List>
  std::variant<List...> as_variant(TypeList<List...>);
  
  template<typename List1, typename List2>
  struct link;
  template<typename ... Args1, typename ... Args2>
  struct link<TypeList<Args1...>, TypeList<Args2...>>
  {
    using type = TypeList<Args1..., Args2...>;
  };
  
  template<typename L1, typename L2>
  using link_t = typename link<L1, L2>::type;
  
  template<typename T, typename List>
  struct contains : std::true_type {};
  template<typename T, typename First, typename... Rest>
  struct contains<T, TypeList<First, Rest...>>
      : std::conditional<std::is_same_v<T, First>, std::true_type,
          contains<T, TypeList<Rest...>>>::type
  {
  };
  template<typename T>
  struct contains<T, TypeList<>> : std::false_type {};
  
  template<typename T, typename List>
  constexpr bool contains_v = contains<T, List>::value;
  
  template<typename T, typename List>
  struct index_of;
  template<typename First, typename ... Rest>
  struct index_of<First, TypeList<First, Rest...>>
  {
    static constexpr int value = 0;
  };
  template<typename T>
  struct index_of<T, TypeList<>>
  {
    static constexpr int value = -1;
  };
  template<typename T, typename First, typename ...Rest>
  struct index_of<T, TypeList<First, Rest...>>
  {
    static constexpr int temp = index_of<T, TypeList<Rest...>>::value;
    static constexpr int value = temp == -1 ? -1 : temp + 1;
  };
  
  template<typename T, typename List>
  constexpr int index_of_v = index_of<T, List>::value;
  
  template<int index, typename List>
  struct index_at;
  template<int index>
  struct index_at<index, TypeList<>>
  {
    using type = TypeListError;
  };
  template<typename First, typename ... Rest>
  struct index_at<0, TypeList<First, Rest...>>
  {
    using type = First;
  };
  template<int index, typename First, typename ... Rest>
  struct index_at<index, TypeList<First, Rest...>>
  {
    using type = typename index_at<index - 1, TypeList<Rest...>>::type;
  };
  
  template<int index, typename List>
  using index_at_t = typename index_at<index, List>::type;
  
  template<typename List, size_t sz = 0>
  struct size_of;
  template<typename First, typename ...Rest, size_t sz>
  struct size_of<TypeList<First, Rest...>, sz>
  {
    static constexpr size_t value = sizeof...(Rest) + 1;
  };
  template<typename List>
  constexpr size_t size_of_v = size_of<List>::value;
  
  class Data;
  
  using MethodParam = std::vector<Data>;
  
  template<typename T>
  concept MethodArgRetType = true;
  
  template<typename T>
  consteval std::string_view qwrpc_type_id()
  {
    std::string_view str = std::experimental::source_location::current().function_name();
    auto b = str.find_first_of('<');
    auto e = str.find_last_of('>');
    return str.substr(b + 1, e - b - 1);
  }
  
  
  class Data
  {
  private:
    std::string data;
    std::string type;
  public:
    Data() = default;
    
    template<typename T>
    requires (!std::is_base_of_v<Data, std::decay_t<T>>)
    T as()
    {
      error::qwrpc_assert(qwrpc_type_id<T>() == type, "Get error type.");
      if constexpr(std::is_same_v<T, void>)
      {
        return;
      }
      else
      {
        return serializer::deserialize<T>(data);
      }
    }
  
    template<typename T>
    requires (!std::is_base_of_v<Data, std::decay_t<T>>)
    Data(T &&value): data(serializer::serialize(std::forward<T>(value))), type(qwrpc_type_id<std::decay_t<T>>()) {}
    
    Data(std::string str, std::string type) : data(std::move(str)), type(std::move(type)) {}
  
    std::string get_type() const { return type; }
  
    std::string get_data() const { return data; }
  };
  
  template<class... Ts>
  struct overloaded : Ts ...
  {
    using Ts::operator()...;
  };
  template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
  
  
  czh::value::Array ret_to_czh_type(const MethodParam &param)
  {
    if (param.empty()) return {};
    error::qwrpc_assert(param.size() == 1);
    czh::value::Array ret;
    ret.emplace_back(param[0].get_type());
    ret.emplace_back(param[0].get_data());
    return ret;
  }
  
  template<typename T>
  T ret_get(const czh::value::Array &ret)
  {
    error::qwrpc_assert(ret.size() == 2);
    error::qwrpc_assert(ret[0].index()
                        == czh::value::details::index_of_v<std::string,
        czh::value::details::BasicVTList>);
    error::qwrpc_assert(std::get<std::string>(ret[0]) == qwrpc_type_id<T>());
    error::qwrpc_assert(ret[1].index()
                        == czh::value::details::index_of_v<std::string,
        czh::value::details::BasicVTList>);
    return Data(std::get<std::string>(ret[1]), std::string(qwrpc_type_id<T>())).as<T>();
  }
  
  template<>
  void ret_get(const czh::value::Array &ret)
  {
    error::qwrpc_assert(ret.empty());
  }
  
  template<typename ...Args>
  auto args_to_czh_array(Args &&...args)
  {
    auto tmp = std::make_tuple(args...);
    // make tuple a MethodParam
    auto param = std::apply([](auto &&... elems)
                            {
                              return MethodParam{static_cast<Data>(elems)...};
                            }, tmp);
    
    // convert MethodParam to czh array
    czh::value::Array ret;
    for (auto &r: param)
    {
      // Data -> std::string[type] + std::string[data]
      ret.emplace_back(r.get_type());
      ret.emplace_back(r.get_data());
    }
    return ret;
  }
  
  template<typename F, typename List, std::size_t... index>
  auto call_with_param_helper(const F &func, const MethodParam &v, std::index_sequence<index...>)
  {
    // convert param to a tuple
    auto tmp = std::make_tuple(v[index]...);
    // convert Data in tmp to the actual Type in List
    auto internal_args = std::apply([](auto &&... elems)
                                    {
                                      return std::make_tuple(elems.template as<index_at_t<index, List>>()...);
                                    }, tmp);
    return func(std::get<index>(internal_args)...);
  }
  
  template<typename ...Args, typename F>
  MethodParam call_with_param(F &&func, const MethodParam &v)
  {
    MethodParam ret;
    ret.emplace_back(call_with_param_helper<F, TypeList<Args...>>
                         (std::forward<F>(func), v, std::make_index_sequence<sizeof...(Args)>()));
    return ret;
  }
  
  template<typename F, typename List, std::size_t... index>
  void call_with_param_void_helper(const F &func, const MethodParam &v, std::index_sequence<index...>)
  {
    // convert param to a tuple
    auto tmp = std::make_tuple(v[index]...);
    // convert Data in tmp to the actual Type in List
    auto internal_args = std::apply([](auto &&... elems)
                                    {
                                      return std::make_tuple(elems.template as<index_at_t<index, List>>()...);
                                    }, tmp);
    func(std::get<index>(internal_args)...);
  }
  
  template<typename ...Args, typename F>
  void call_with_param_void(F &&func, const MethodParam &v)
  {
    call_with_param_helper<F, TypeList<Args...>>
        (std::forward<F>(func), v, std::make_index_sequence<sizeof...(Args)>());
  }
  
  template<typename ...Args>
  auto make_index()
  {
    // CustomType -> -1
    return std::vector<std::string>{
        {
            std::string(qwrpc_type_id<std::decay_t<Args>>())
        }...};
  }
  
  class Method
  {
  private:
    std::function<MethodParam(MethodParam)> func;
    std::vector<std::string> args;
    std::string ret_type;
  public:
    Method() = default;
  
    template<MethodArgRetType ...Args, MethodArgRetType Ret>
    Method(std::function<Ret(Args...)> f)
        :args(make_index<std::decay_t<Args>...>()),
         func([f](MethodParam call_args)
              {
                if constexpr(std::is_same_v<std::decay_t<Ret>, void>)
                {
                  call_with_param_void<std::decay_t<Args>...>(f, call_args);
                  return MethodParam{};
                }
                else
                {
                  return call_with_param<std::decay_t<Args>...>(f, call_args);
                }
              }),
         ret_type(qwrpc_type_id<Ret>()) {}
    
    bool check_args(const czh::value::Array &call_args) const
    {
      if (call_args.size() % 2 != 0) return false;
      if (call_args.size() / 2 != args.size()) return false;
      for (size_t i = 0; i < args.size(); i++)
      {
        if (call_args[i * 2].index() != czh::value::details::index_of_v<std::string, czh::value::details::BasicVTList>)
        {
          return false;
        }
        if (call_args[i * 2 + 1].index() !=
            czh::value::details::index_of_v<std::string, czh::value::details::BasicVTList>)
        {
          return false;
        }
        if (std::get<std::string>(call_args[i * 2]) != args[i])
        {
          return false;
        }
      }
      return true;
    }
  
    bool check_ret(const std::string &ret) const
    {
      return ret == ret_type;
    }
    
    MethodParam call(const czh::value::Array &call_args) const
    {
      MethodParam internal_args;
      for (size_t i = 0; i < call_args.size(); i += 2)
      {
        Data data(std::get<std::string>(call_args[i + 1]), std::get<std::string>(call_args[i]));
        internal_args.emplace_back(std::move(data));
      }
      return func(std::move(internal_args));
    }
    
    czh::value::Array expected_args() const
    {
      czh::value::Array ret;
      for (auto &r: args)
      {
        ret.emplace_back(r);
      }
      return ret;
    }
    
    std::string expected_ret() const
    {
      return ret_type;
    }
  };
}
#endif
