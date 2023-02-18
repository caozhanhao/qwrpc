<h2 align="center">
qwrpc
</h2> 

<p align="center">
<strong>一个易用的Modern C++ header-only RPC库.</strong>
</p>

<p align="center">
  <a href="https://github.com/caozhanhao/libczh" >
    <img src="https://img.shields.io/static/v1?label=libczh&message=czh&color=blue&style=flat-square" alt="libczh" />  
  </a>
  <a href="LICENSE" >
    <img src="https://img.shields.io/github/license/caozhanhao/qwrpc?label=License&style=flat-square&color=yellow" alt="License" />  
  </a>
</p>

<p align="center">
  <a href="README.md" >
    <img src="https://img.shields.io/badge/Document-English-blue" alt="Document" />  
  </a>
  <a href="README-cn.md" >
    <img src="https://img.shields.io/badge/文档-简体中文-blue" alt="Document" />  
  </a>
</p>

- Header only
- 易用的接口
- 运行时类型检查

### 一个简单的例子

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

- `cli.call<T>` 返回 `T`
- `cli.async_call<...>` 返回一个 `std::future<T>`

### 更多

#### 类型支持

qwrpc支持以下类型

- 任何符合 `std::is_trivially_copyable` 的类型
- 任何有serialize和deserialize特化的类型（见下方）
- 任何存储已支持类型的容器(有 begin(), end(), insert())

值得注意的是，上面第三个规则指出像`std::vector<std::vector<...>>`也是支持的

如果还需要更多类型，添加`qwrpc::serializer::serialize()` 和 `qwrpc::serializer::deserialize()`的特化。

- serialize和deserialize

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

#### 注册方法

任何符合以下规则的函数都可以直接注册。

- 参数和返回值是已支持类型(见上方)

```c++
svr.register_method("plus", std::plus<int>());
svr.register_method("plus",[](int a, int b){ return a + b;});
svr.register_method("foo",
                      [](std::vector<qwrpc_example::C> c) -> std::vector<std::vector<qwrpc_example::D>>
                      {
                        // do something
                      });
```

更多例子请看[examples](examples/).

#### 日志

init_logger(minimum severity, output mode, filename(opt))

- minimum severity: NONE, TRACE, DEBUG, INFO, WARN, ERR, CRITICAL.
- output mode: file, console, file_and_console, none
- filename: 仅当output mode是file或者file_and_console的时候才需要filename。
- 如果你不需要日志，只需要不使用init_logger()或者把output mode设为none

```c++
qwrpc::logger::init_logger(qwrpc::logger::Severity::NONE,
                           qwrpc::logger::Output::file_and_console,
                           "qwrpc_server_log.txt");
```

### 依赖

- [libczh](https://github.com/caozhanhao/libczh)
- 需要 C++ 20

## 联系

- 如果你有任何问题或建议，请提交一个issue或给我发邮件
- 邮箱: cao2013zh at 163 dot com

## 贡献

- 欢迎任何贡献，提一个PR就可以

## 许可

- qwrpc 根据 [Apache-2.0 license](LICENSE)获得许可