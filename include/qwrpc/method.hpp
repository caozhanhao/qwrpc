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
  
  class Data
  {
  private:
    std::string data;
  public:
    Data() = default;
    
    template<typename T>
    requires (!std::is_same_v<std::decay_t<T>, MethodParamV> && !std::is_same_v<std::decay_t<T>, std::string>
              && !std::is_base_of_v<Data, std::decay_t<T>>)
    operator T()
    {
      return serializer::deserialize<T>(data);
    }
    
    template<typename T>
    requires (!std::is_same_v<std::decay_t<T>, std::string> && !std::is_base_of_v<Data, std::decay_t<T>>)
    Data(T &&value): data(serializer::serialize(std::forward<T>(value))) {}
    
    Data(std::string str) : data(std::move(str)) {}
    
    std::string as_string() const
    {
      return data;
    }
  };
  
  template<typename T>
  consteval std::string_view nameof()
  {
    std::string_view str = std::experimental::source_location::current().function_name();
    auto b = str.find_first_of('<');
    auto e = str.find_last_of('>');
    return str.substr(b + 1, e - b - 1);
  }
  
  std::string get_typename(size_t sz)
  {
    static std::vector<std::string>
        names{"int", "long long", "double", "bool", "std::string", "Data"};
    return names[sz];
  }
  
  
  template<class... Ts>
  struct overloaded : Ts ...
  {
    using Ts::operator()...;
  };
  template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
  
  // the std::string after czh::value::Null should be converted to Data
  MethodParam czh_array_to_param(const czh::value::Array &arr)
  {
    MethodParam ret;
    bool find_null = false;
    for (auto &r: arr)
    {
      std::visit(overloaded{
          [&ret](auto &&item) { ret.emplace_back(item); },
          [&find_null](czh::value::Null) { find_null = true; },
          [&ret, &find_null](std::string str)
          {
            if (find_null)
            {
              Data data(str);
              ret.emplace_back(std::move(data));
              find_null = false;
            }
            else
            {
              ret.emplace_back(str);
            }
          }
      }, r);
    }
    return ret;
  }
  
  czh::value::Array param_to_czh_array(const MethodParam &param)
  {
    czh::value::Array ret;
    for (auto &r: param)
    {
      std::visit(overloaded{
          [&ret](auto &&item) { ret.emplace_back(item); },
          [&ret](const Data &item)
          {
            ret.emplace_back(czh::value::Null{});
            ret.emplace_back(item.as_string());
          }
      }, r);
    }
    return ret;
  }
  
  template<typename List, std::size_t... index>
  auto param_to_tuple_helper(const MethodParam &v, std::index_sequence<index...>)
  {
    // convert param to a tuple
    auto tmp = std::make_tuple(
        std::get<
            std::conditional_t<index_of_v<index_at_t<index, List>, MethodParamList> == -1,
                Data,
                index_at_t<index, List>
            >>(v[index])...);
    // replace Data in tmp with the actual CustomType in List
    return std::apply([](auto &&... elems)
                      {
                        return std::make_tuple(
                            static_cast<std::conditional_t<
                                std::is_same_v<std::decay_t<decltype(elems)>, Data>,
                                index_at_t<index, List>,
                                decltype(std::forward<decltype(elems)>(elems))
                            >>(elems)...);
                      }, tmp);
  }
  
  template<typename List, std::size_t N>
  auto param_to_tuple(const MethodParam &v)
  {
    return param_to_tuple_helper<List>(v, std::make_index_sequence<N>());
  }
  
  // adapted from: https://stackoverflow.com/questions/42494715/c-transform-a-stdtuplea-a-a-to-a-stdvector-or-stddeque
  template<typename Tuple>
  MethodParam tuple_to_param(Tuple &&tuple)
  {
    // replace CustomType to Data
    return std::apply([](auto &&... elems)
                      {
                        return MethodParam{
                            static_cast<std::conditional_t<
                                index_of_v<std::decay_t<decltype(elems)>, MethodParamList> == -1,
                                Data,
                                decltype(std::forward<decltype(elems)>(elems))
                            >>(elems)...};
                      }, std::forward<Tuple>(tuple));
  }
  
  template<typename Tuple>
  std::vector<int> tuple_to_index(Tuple &&tuple)
  {
    // CustomType -> -1
    return std::apply([](auto &&... elems)
                      {
                        return std::vector<int>{
                            index_of_v<std::decay_t<decltype(elems)>, MethodParamList>...};
                      }, std::forward<Tuple>(tuple));
  }
  
  template<typename ...T>
  using MethodRets = std::tuple<T...>;
  template<typename ...T>
  using MethodArgs = std::tuple<T...>;
  
  template<typename T>
  struct make_method_helper;
  template<typename Ret, typename T, typename... Args>
  struct make_method_helper<Ret(T::*)(Args...) const>
  {
    using type = std::function<Ret(Args...)>;
  };
  
  template<typename F>
  typename make_method_helper<decltype(&F::operator())>::type
  make_method(F const &m)
  {
    return m;
  }
  
  class Method
  {
  private:
    std::function<MethodParam(MethodParam)> func;
    std::vector<int> args;
  public:
    Method() = default;
    
    template<typename ...Args, typename ...Rets>
    Method(std::function<MethodRets<Rets...>(MethodArgs<Args...>)> f)
        :args(tuple_to_index(MethodArgs<Args...>{}))
    {
      func = [f](MethodParam args) -> MethodParam
      {
        auto internal_args = param_to_tuple<TypeList<Args...>, sizeof...(Args)>(args);
        auto internal_ret = f(internal_args);
        auto ret = tuple_to_param(internal_ret);
        return ret;
      };
    }
    
    bool check_args(const czh::value::Array &call_args) const
    {
      if (call_args.size() < args.size()) return false;
      size_t j = 0;
      for (size_t i = 0; i < args.size(); ++i, ++j)
      {
        // CustomType -> Data -> Null + std::string, see above
        // IOW, args{-1} -> call_args{Null, std::string}
        if (args[i] == -1)
        {
          if (call_args[j].index() == 0// Null
              && j + 1 < call_args.size()
              && call_args[j + 1].index() == 5)// std::string
          {
            ++j;
            continue;
          }
          else { return false; }
        }
        else if (args[i] != call_args[j].index() - 1)
        {
          return false;
        }
      }
      if (j != call_args.size()) return false;
      return true;
    }
    
    MethodParam call(MethodParam call_args) const
    {
      return func(std::move(call_args));
    }
    
    czh::value::Array expected_args() const
    {
      czh::value::Array ret;
      for (auto &r: args)
      {
        if (r == -1)
        {
          ret.emplace_back(get_typename(5));// Data
        }
        else
        {
          ret.emplace_back(get_typename(r));
        }
      }
      return ret;
    }
  };
}
#endif
