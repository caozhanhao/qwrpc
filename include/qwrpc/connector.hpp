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
#ifndef QWRPC_CONNECTOR_HPP
#define QWRPC_CONNECTOR_HPP
#pragma once

#include "error.hpp"
#include <unistd.h>
#include <sys/types.h>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#endif

#include <cstring>
#include <functional>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <memory>
#include <exception>

namespace qwrpc::error::connector
{
  constexpr auto socket_init_error = "socket create error";
  constexpr auto socket_bind_error = "socket bind error";
  constexpr auto socket_accept_error = "socket accept error";
  constexpr auto socket_listen_error = "socket listen error";
  constexpr auto socket_connect_error = "socket connect error";
  constexpr auto socket_recv_error = "socket recv error";
  constexpr auto socket_send_error = "socket send_and_recv error";
}
namespace qwrpc::connector
{
  constexpr int MAGIC = 0x18273645;
#ifdef _WIN32
  WSADATA qwrpc_wsa_data;
  [[maybe_unused]] int wsa_startup_err = WSAStartup(MAKEWORD(2,2),&qwrpc_wsa_data);
#endif
  
  class Thpool
  {
  private:
    using Task = std::function<void()>;
    std::vector<std::thread> pool;
    std::queue<Task> tasks;
    std::atomic<bool> run;
    std::mutex th_mutex;
    std::exception_ptr err_ptr;
    std::condition_variable cond;
  public:
    explicit Thpool(std::size_t size) : run(true) { add_thread(size); }
    
    ~Thpool()
    {
      run = false;
      cond.notify_all();
      for (auto &th: pool)
      {
        if (th.joinable()) th.join();
      }
    }
    
    template<typename Func, typename... Args>
    auto add_task(Func &&f, Args &&... args)
    -> std::future<typename std::result_of<Func(Args...)>::type>
    {
      error::qwrpc_assert(run, "Can not add task on stopped Thpool");
      using ret_type = typename std::result_of<Func(Args...)>::type;
      auto task = std::make_shared<std::packaged_task<ret_type() >>
          (std::bind(std::forward<Func>(f),
                     std::forward<Args>(args)...));
      std::future<ret_type> ret = task->get_future();
      {
        std::lock_guard<std::mutex> lock(th_mutex);
        tasks.emplace([task] { (*task)(); });
      }
      cond.notify_one();
      return ret;
    }
    
    void add_thread(std::size_t num)
    {
      for (std::size_t i = 0; i < num; i++)
      {
        pool.emplace_back(
            [this]
            {
              while (this->run)
              {
                Task task;
                {
                  std::unique_lock<std::mutex> lock(this->th_mutex);
                  this->cond.wait(lock, [this] { return !this->run || !this->tasks.empty(); });
                  if (!this->run && this->tasks.empty()) return;
                  task = std::move(this->tasks.front());
                  this->tasks.pop();
                }
                task();
              }
            }
        );
      }
    }
  };
  
  struct Msg
  {
    int32_t magic;
    uint64_t content_length;
  };
  
  struct Addr
  {
    struct sockaddr_in addr;
#ifdef _WIN32
    int len;
#else
    socklen_t len;
#endif
    
    Addr() : len(sizeof(addr))
    {
      std::memset(&addr, 0, sizeof(addr));
    }
    
    Addr(const std::string &ip, int port)
    {
      std::memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = inet_addr(ip.c_str());
      addr.sin_port = htons(port);
      len = sizeof(addr);
    }
  };
  
  
  class Socket
  {
  private:
#ifdef _WIN32
    SOCKET fd;
#else
    int fd;
#endif
  public:
    Socket() : fd(-1)
    {
      fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      int on = 1;
      setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&on), sizeof(on));
      error::qwrpc_assert(fd != -1, error::connector::socket_init_error);
    }

#ifdef _WIN32
    Socket(SOCKET fd_) : fd(fd_) {}
#else
    
    Socket(int fd_) : fd(fd_) {}

#endif
    
    Socket(const Socket &) = delete;
    
    Socket(Socket &&soc) : fd(soc.fd)
    {
      soc.fd = -1;
    }
    
    ~Socket()
    {
      if (fd != -1)
      {
        ::close(fd);
        fd = -1;
      }
    }
    
    std::tuple<Socket, Addr> accept() const
    {
      Addr addr;
#ifdef _WIN32
      return {std::move(Socket{::accept(fd, reinterpret_cast<sockaddr *>(&addr.addr),
                                        reinterpret_cast<int *> (&addr.len))}), addr};
#else
      return {std::move(Socket{::accept(fd, reinterpret_cast<sockaddr *>(&addr.addr),
                                        reinterpret_cast<socklen_t *>(&addr.len))}), addr};
#endif
    }
    
    int get_fd() const { return fd; }
    
    void send(const std::string &str) const
    {
      Msg msg{.magic = MAGIC, .content_length = str.size()};
      error::qwrpc_assert(::send(fd, reinterpret_cast<char *>(&msg), sizeof(Msg), 0) >= 0,
                          error::connector::socket_send_error);
      error::qwrpc_assert(::send(fd, str.data(), str.size(), 0) >= 0,
                          error::connector::socket_send_error);
    }
    
    std::string recv() const
    {
      Msg msg_recv;
      error::qwrpc_assert(
          ::recv(fd, reinterpret_cast<char *>(&msg_recv), sizeof(Msg), 0) == sizeof(Msg)
          && msg_recv.magic == MAGIC, error::connector::socket_recv_error);
      std::string recv_result(msg_recv.content_length, 0);
      error::qwrpc_assert(::recv(fd, &recv_result[0], msg_recv.content_length, 0) == msg_recv.content_length,
                          error::connector::socket_recv_error);
      return recv_result;
    }
    
    void bind(Addr addr) const
    {
      error::qwrpc_assert(::bind(fd, (sockaddr *) &addr.addr, addr.len) == 0,
                          error::connector::socket_bind_error);
    }
    
    void listen() const
    {
      error::qwrpc_assert(::listen(fd, 0) != -1, error::connector::socket_listen_error);
    }
    
    void connect(Addr addr) const
    {
      error::qwrpc_assert(::connect(fd, (sockaddr *) &addr.addr, addr.len) != -1,
                          error::connector::socket_connect_error);
    }
  };
  
  class Server
  {
  private:
    int port;
    bool running;
    std::function<void(const std::string &, std::string &)> router;
    Thpool thpool;
  public:
    Server(int p, const std::function<void(const std::string &, std::string &)> &router_)
        : port(p), router(router_), thpool(16) {}
    
    void start()
    {
      Socket socket;
      socket.bind({"127.0.0.1", port});
      socket.listen();
      while (running)
      {
        auto tmp = socket.accept();
        auto&[clnt_socket, clnt_addr] = tmp;
        error::qwrpc_assert(clnt_socket.get_fd() != -1, error::connector::socket_accept_error);
        thpool.add_task(
            [this, clnt_socket = std::move(std::get<0>(tmp))]
            {
              while (true)
              {
                auto request = clnt_socket.recv();
                if (request == "quit")
                {
                  return;
                }
                std::string response;
                router(request, response);
                clnt_socket.send(response);
              }
            });
      }
    }
  };
  
  class Client
  {
  private:
    Socket socket;
  public:
    ~Client()
    {
      socket.send("quit");
    }
    
    void connect(const std::string &addr, int port)
    {
      socket.connect({addr, port});
    }
  
    std::string send_and_recv(const std::string &str)
    {
      socket.send(str);
      return socket.recv();
    }
  };
}
#endif