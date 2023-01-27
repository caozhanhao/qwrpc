# qwrpc

[![License](https://img.shields.io/github/license/caozhanhao/qwrpc?label=License&style=flat-square)](LICENSE)

A simple C++ 20 header-only RPC library.

- Not finished yet.

### Setup

- just include the `qwrpc/qwrpc.hpp`!

### A Simple Example

#### RpcServer

```c++
  qwrpc::RpcServer svr;
svr.register_method("add",
  [](qwrpc::MethodArgs<int, int> args)
    -> qwrpc::MethodRets<int>
  {
    auto[rhs, lhs] = args;     
    return {rhs + lhs};   
  });
```

- `"add"` is the method id.
- `qwrpc::MethodArgs<...>` is the method's arguments(aka. `std::tuple<...>`)
- `qwrpc::MethodRets<...>` is the method's return value(aka. `std::tuple<...>`)

#### RpcClient

```c++
  qwrpc::RpcClient cli("127.0.0.1:8765");
auto add_ret = cli.call<int>("add", 1, 1);
  // or use async_call<int>("add", 1, 1);
  auto[add] = add_ret;
  std::cout << "add: " << add << std::endl; // 2
```

- `cli.call<...>` means it returns `qwrpc::MethodRets<...>`(aka. `std::tuple<...>`)
- `cli.async_call<...>` returns a `std::future<qwrpc::MethodRets<...>>`
- its arguments (`1, 1`) are the method's arguments, which is the rpc_server's `qwrpc::MethodArgs<...>`

### More

#### Custom Type

`qwrpc::MethodRets<...>` and `qwrpc::MethodArgs<...>` only support `int, long long, double, bool, std::string`, add
specialization of `qwrpc::serializer::serialize()` and `qwrpc::serializer::deserialize()` to enable more type.

- serialize

```c++
  template<>
  std::string serialize(const custom_type& b)
  {
    // do something
  }
```

- deserialize

```c++
  template<>
  custom_type deserialize(const std::string& str)
  {
    // do something
  }
```

- the rpc_server and rpc_client are the same as previous examples.

- For more examples, please see [examples](examples/).

### Dependencies

- [libczh](https://github.com/caozhanhao/libczh)
- Requires C++ 20
