# qwrpc
[![License](https://img.shields.io/github/license/caozhanhao/qwrpc?label=License&style=flat-square)](LICENSE)   
A simple C++ header-only RPC.

### A Simple Example
#### Server

```c++
  qwrpc::Server svr;
svr.register_method("add",
qwrpc::make_method([](qwrpc::MethodArgs<int, int> args) -> qwrpc::MethodRets<int>
{
auto[rhs, lhs] = args;
return { rhs + lhs };
}));
```

- `"add"` is the method id.
- `qwrpc::MethodArgs<...>` is the method's arguments(aka. `std::tuple<...>`)
- `qwrpc::MethodRets<...>` is the method's return value(aka. `std::tuple<...>`)
- use `qwrpc::make_method` to convert the lambda to qwrpc::Method

#### Client

```c++
  qwrpc::Client cli("127.0.0.1:8765");
auto add_ret = cli.call<int>("add", 1, 1);
auto[add] = add_ret;
std::cout << "add: " << add << std::endl; // 2
```

- `cli.call<...>` means it returns `qwrpc::MethodRets<...>`(aka. `std::tuple<...>`)
- its arguments (`1, 1`) are the method's arguments, which is the server's `qwrpc::MethodArgs<...>`

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

- the server and client are similar to previous example, see below

##### server

```c++
  svr.register_method("example",
qwrpc::make_method([](qwrpc::MethodArgs<custom_type> args)
-> qwrpc::MethodRets<custom_type>
{
// do something 
}));
```

##### client

```c++
  auto ret = cli.call<custom_type>("example", custom_type{ "example" });
```

More examples see [examples](examples/)

### Build

- just `#include "qwrpc/qwrpc.hpp"`

### Dependency

- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [libczh](https://github.com/caozhanhao/libczh)
- Requires C++ 20