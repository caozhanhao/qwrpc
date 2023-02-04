#ifndef QWRPC_EXAMPLE_HPP
#define QWRPC_EXAMPLE_HPP
#pragma once

#include "qwrpc/qwrpc.hpp"
#include <iostream>

namespace qwrpc_example
{
  class A
  {
  private:
    int data;
  public:
    A(int i) : data(i) {}
  
    ~A() {};// make it not trivially copyable
  
    int get_data() const { return data; }
  };
  
  class B
  {
  private:
    std::string data;
  public:
    ~B() {};// make it not trivially copyable
    
    B(std::string i) : data(i) {}
    
    void print() { std::cout << data << std::endl; }
    
    std::string get_data() const { return data; }
  };
  
  struct C { int c; };
  struct D { int d; };
}
namespace qwrpc::serializer
{
  template<>
  std::string serialize(const qwrpc_example::A &a)
  {
    return std::to_string(a.get_data());
  }
  
  template<>
  qwrpc_example::A deserialize(const std::string &str)
  {
    int a;
    try
    {
      a = (std::stoi(str));
    }
    catch (...)
    {
      throw qwrpc::error::Error("Invalid string.");
    }
    return {a};
  }
  
  template<>
  std::string serialize(const qwrpc_example::B &b)
  {
    return b.get_data();
  }
  
  template<>
  qwrpc_example::B deserialize(const std::string &str)
  {
    return {str};
  }
}
#endif
