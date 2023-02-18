<h2 align="center">
qwrpc
</h2> 

<p align="center">
<strong>A easy-to-use Modern C++ header-only RPC library.</strong>
</p>

<p align="center">
  <a href="https://github.com/caozhanhao/libczh" >
    <img src="https://img.shields.io/static/v1?label=libczh&message=czh&color=blue&style=flat-square" alt="libczh" />  
  </a>
  <a href="LICENSE" >
    <img src="https://img.shields.io/github/license/caozhanhao/qwrpc?label=License&style=flat-square&color=yellow" alt="License" />  
  </a>
</p>

- Header only
- Easy-to-use interface
- Runtime type checking

### A Simple Example

#### RpcServer

```c++
qwrpc::RpcServer svr(8765);
svr.register_method("plus", std::plus<int>());
```

#### RpcClient

```c++
qwrpc::RpcClient cli("127.0.0.1:8765");
auto ret = cli.call<int>("plus", 1, 1);
// or async_call<int>("plus", 1, 1);
```

- `cli.call<T>` returns `T`
- `cli.async_call<...>` returns a `std::future<T>`

### More

#### Type Support

qwrpc supports

- any types that satisfied `std::is_trivially_copyable`
- any types with specialization of serialize and deserialize(see below)
- any containers(with begin(), end(), insert()) whose value's type is supported types above

Rule 3 means something like `std::vector<std::vector<int>>` is also supported. If more types are needed, add
specialization `qwrpc::serializer::serialize()` and `qwrpc::serializer::deserialize()`

- serialize and deserialize

```c++
namespace qwrpc::serializer
{
  template<>
  std::string serialize(const custom_type& b)
  {
    // do something
  }
  template<>
  custom_type deserialize(const std::string& str)
  {
    // do something
  }
}
```

#### Register

Any functions satisfied the following rule can be used directly.

- Parameters and return value is the supported types.(see above)

```c++
svr.register_method("plus", std::plus<int>());
svr.register_method("plus",[](int a, int b){ return a + b;});
svr.register_method("foo",
                      [](std::vector<qwrpc_example::C> c) -> std::vector<std::vector<qwrpc_example::D>>
                      {
                        // do something
                      });
```

For more examples, please see [examples](examples/).

#### Logger

- init_logger(minimum severity, output mode, filename(opt))
- minimum severity: NONE, TRACE, DEBUG, INFO, WARN, ERR, CRITICAL.
- output mode: file, console, file_and_console, none
- filename: Only when output mode is file or file_and_console, filename is need.
- If you don't want logger, just don't use init_logger() or simply set output mode to none.

```c++
qwrpc::logger::init_logger(qwrpc::logger::Severity::NONE,
                           qwrpc::logger::Output::file_and_console,
                           "qwrpc_server_log.txt");
```

### Dependencies

- [libczh](https://github.com/caozhanhao/libczh)
- Requires C++ 20

### Contact

- If you have any questions or suggestions, please submit an issue or email me.
- Email: cao2013zh at 163 dot com

### Contribution

- Any contributions are welcomed, just send a PR.

### License

- qwrpc is licensed under the [Apache-2.0 license](LICENSE)
