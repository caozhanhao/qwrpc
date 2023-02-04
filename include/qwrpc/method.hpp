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
  
  using MethodParamList = TypeList<int, long long, double, bool, std::string, Data>;
  using MethodParamV = decltype(as_variant(MethodParamList{}));
  using MethodParam = std::vector<MethodParamV>;
  
  template<typename T>
  consteval std::string_view nameof()
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
    requires (!std::is_same_v<std::decay_t<T>, MethodParamV> && !std::is_same_v<std::decay_t<T>, std::string>
              && !std::is_base_of_v<Data, std::decay_t<T>>)
    T as()
    {
      error::qwrpc_assert(nameof<T>() == type, "Get error type.");
      return serializer::deserialize<T>(data);
    }
    
    template<typename T>
    requires (!std::is_same_v<std::decay_t<T>, std::string> && !std::is_base_of_v<Data, std::decay_t<T>>)
    Data(T &&value): data(serializer::serialize(std::forward<T>(value))), type(nameof<std::decay_t<T>>()) {}
    
    Data(std::string str, std::string type) : data(std::move(str)), type(std::move(type)) {}
    
    std::string as_string() const
    {
      return data;
    }
  
    std::string get_type() const { return type; }
  };
  
  template<class... Ts>
  struct overloaded : Ts ...
  {
    using Ts::operator()...;
  };
  template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
  
  
  czh::value::Array ret_to_czh_type(const MethodParam &param)
  {
    error::qwrpc_assert(param.size() == 1);
    czh::value::Array ret;
    std::visit(overloaded{
        [&ret](auto &&item)
        {
          ret.emplace_back(item);
        },
        [&ret](const Data &item)
        {
          ret.emplace_back(czh::value::Null{});
          ret.emplace_back(item.get_type());
          ret.emplace_back(item.as_string());
        },
    }, param[0]);
    return ret;
  }
  
  template<typename T>
  T ret_get(const czh::value::Array &ret)
  {
    if constexpr(czh::value::details::index_of_v<T, czh::value::details::BasicVTList> == -1)
    {
      error::qwrpc_assert(ret.size() == 3);
      error::qwrpc_assert(ret[0].index() == czh::value::details::index_of_v<czh::value::Null,
          czh::value::details::BasicVTList>);
      error::qwrpc_assert(ret[1].index()
                          == czh::value::details::index_of_v<std::string,
          czh::value::details::BasicVTList>);
      error::qwrpc_assert(std::get<std::string>(ret[1]) == nameof<T>());
      error::qwrpc_assert(ret[2].index()
                          == czh::value::details::index_of_v<std::string,
          czh::value::details::BasicVTList>);
      return Data(std::get<std::string>(ret[2]), std::string(nameof<T>())).as<T>();
    }
    else
    {
      error::qwrpc_assert(ret.size() == 1);
      return std::get<T>(ret[0]);
    }
  }
  
  template<typename ...Args>
  auto args_to_czh_array(Args &&...args)
  {
    auto tmp = std::make_tuple(args...);
    // replace CustomType to Data and make tuple a MethodParam
    auto param = std::apply([](auto &&... elems)
                            {
                              return MethodParam{
                                  static_cast<std::conditional_t<
                                      index_of_v<std::decay_t<decltype(elems)>, MethodParamList> == -1,
                                      Data,
                                      decltype(std::forward<decltype(elems)>(elems))
                                  >>(elems)...};
                            }, tmp);
    
    // convert MethodParam to czh array
    czh::value::Array ret;
    for (auto &r: param)
    {
      std::visit(overloaded{
          [&ret](auto &&item) { ret.emplace_back(item); },
          [&ret](Data &item)
          {
            // Data -> Null + std::string[type] + std::string[data]
            ret.emplace_back(czh::value::Null{});
            ret.emplace_back(item.get_type());
            ret.emplace_back(item.as_string());
          }
      }, r);
    }
    return ret;
  }
  
  template<typename To, typename From>
  auto data_conv(From &&data)
  {
    if constexpr(std::is_same_v<std::decay_t<From>, Data>)
    {
      return data.template as<To>();
    }
    else
    {
      return std::forward<To>(data);
    }
  }
  
  template<typename F, typename List, std::size_t... index>
  auto call_with_param_helper(const F &func, const MethodParam &v, std::index_sequence<index...>)
  {
    // convert param to a tuple
    auto tmp = std::make_tuple(
        std::get<
            std::conditional_t<index_of_v<index_at_t<index, List>, MethodParamList> == -1,
                Data,
                index_at_t<index, List>
            >>(v[index])...);
    // convert Data in tmp to the actual CustomType in List
    auto internal_args = std::apply([](auto &&... elems)
                                    {
                                      return std::make_tuple(
                                          data_conv<index_at_t<index, List>>(elems)...);
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
  
  template<typename ...Args>
  auto make_index()
  {
    auto tmp = std::tuple<Args...>();
    // CustomType -> -1
    return std::apply([](auto &&... elems)
                      {
                        return std::vector<std::tuple<int, std::string>>{
                            {
                                index_of_v<std::decay_t<decltype(elems)>, MethodParamList>,
                                std::string(nameof<std::decay_t<decltype(elems)>>())
                            }...};
                      }, tmp);
  }
  
  class Method
  {
  private:
    std::function<MethodParam(MethodParam)> func;
    std::vector<std::tuple<int, std::string>> args;
    std::string ret_type;
  public:
    Method() = default;
    
    template<typename ...Args, typename Ret>
    Method(std::function<Ret(Args...)> f)
        :args(make_index<std::decay_t<Args>...>()),
         func([f](MethodParam call_args) { return call_with_param<std::decay_t<Args>...>(f, call_args); }),
         ret_type(nameof<Ret>()) {}
    
    bool check_args(const czh::value::Array &call_args) const
    {
      if (call_args.size() < args.size()) return false;
      size_t j = 0;
      for (size_t i = 0; i < args.size(); ++i, ++j)
      {
        // CustomType -> Data -> Null + std::string(type) + std::string(data), see above
        // IOW, args{{-1, std::string}} -> call_args{Null, std::string, std::string}
        auto&[index, name] = args[i];
        if (index == -1)
        {
          if (j + 2 >= call_args.size()) return false;
          auto null = std::get_if<czh::value::Null>(&call_args[j]);
          auto type = std::get_if<std::string>(&call_args[j + 1]);
          auto data = std::get_if<std::string>(&call_args[j + 2]);
          if (null != nullptr && type != nullptr && data != nullptr && *type == name)
          {
            j += 2;
            i += 1;
            continue;
          }
          else
          {
            return false;
          }
        }
        else if (index != call_args[j].index() - 1)
        {
          return false;
        }
      }
      if (j != call_args.size()) return false;
      return true;
    }
    
    bool check_ret(const std::string &ret) const
    {
      return ret == ret_type;
    }
    
    MethodParam call(const czh::value::Array &call_args) const
    {
      MethodParam internal_args;
      bool find_null = false;
      std::string type;
      for (auto &r: call_args)
      {
        std::visit(overloaded{
            [&internal_args](auto &&item) { internal_args.emplace_back(item); },
            [&find_null](czh::value::Null) { find_null = true; },
            [&internal_args, &find_null, &type](std::string str)
            {
              // Data -> Null + std::string(type) + std::string(data)
              if (find_null)
              {
                if (type.empty())
                {
                  type = std::move(str);
                  return;
                }
                Data data(std::move(str), type);
                internal_args.emplace_back(std::move(data));
                type.clear();
                find_null = false;
              }
              else
              {
                internal_args.emplace_back(str);
              }
            }
        }, r);
      }
      return func(std::move(internal_args));
    }
    
    czh::value::Array expected_args() const
    {
      czh::value::Array ret;
      for (auto &r: args)
      {
        ret.emplace_back(std::get<1>(r));
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
