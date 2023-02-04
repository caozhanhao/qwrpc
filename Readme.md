# qwrpc

[![License](https://img.shields.io/github/license/caozhanhao/qwrpc?label=License&style=flat-square)](LICENSE)

A easy-to-use Modern C++ header-only RPC library.

### Setup

- just include the `qwrpc/qwrpc.hpp`!

### A Simple Example

#### RpcServer

```c++
  qwrpc::RpcServer svr(8765);
svr.register_method("plus", std::plus<int>());
// or 
svr.register_method("plus",[](int a, int b){ return a + b;});
```

- `"plus"` is the method id.

#### RpcClient

```c++
  qwrpc::RpcClient cli("127.0.0.1:8765");
auto add_ret = cli.call<int>("plus", 1, 1);
// or use async_call<int>("plus", 1, 1);
std::cout << "plus: " << add_ret << std::endl; // 2
```

- `cli.call<T>` means it returns `T`)
- `cli.async_call<...>` returns a `std::future<T>`

### More

#### Type Support

qwrpc originally supports

- `int`
- `long long`
- `double`
- `bool`
- `std::string`
- types satisfied `std::is_trivially_copyable`

If more types are needed, add specialization `qwrpc::serializer::serialize()` and `qwrpc::serializer::deserialize()`

- serialize

```c++
namespace qwrpc::serializer
{
  template<>
  std::string serialize(const custom_type& b)
  {
    // do something
  }
}
```

- deserialize

```c++
namespace qwrpc::serializer
{
  template<>
  custom_type deserialize(const std::string& str)
  {
    // do something
  }
}
```

#### Register

Any functions satisfied the following rule can be used directly.

- Parameters and return value is the supported types or any types with serializer.

```c++
  svr.register_method("plus", std::plus<int>());
svr.register_method("plus",[](int a, int b){ return a + b;});
```

For more examples, please see [examples](examples/).

### Dependencies

- [libczh](https://github.com/caozhanhao/libczh)
- Requires C++ 20
