# qwrpc - A simple C++ RPC

#### Dependency

- cpp-httplib
- libczh
- Requires C++ 20

#### Server

- a simple example from `examples/server.cpp`

```c++
  svr.register_method("add",
qwrpc::method([](qwrpc::MethodArgs<int, int> args)
-> qwrpc::MethodRets<int>
{
auto[rhs, lhs] = args;
return { rhs + lhs };
}));
```

#### Client

```c++
  qwrpc::Client cli("127.0.0.1:8765");
auto ret = cli.call<int>("add", 1, 1);
auto[a] = ret;
// a == 2
```

This project is not finished yet.
