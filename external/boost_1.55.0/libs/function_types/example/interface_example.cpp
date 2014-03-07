
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------
// See interface.hpp in this directory for details.

#include <iostream>
#include <typeinfo>

#include "interface.hpp"


BOOST_EXAMPLE_INTERFACE( interface_x,
  (( a_func, (void)(int) , const_qualified ))
  (( a_func, (void)(long), const_qualified ))
  (( another_func, (int) , non_const   )) 
);


// two classes that implement interface_x

struct a_class
{
  void a_func(int v) const
  {
    std::cout << "a_class::void a_func(int v = " << v << ")" << std::endl;
  }

  void a_func(long v) const
  {
    std::cout << "a_class::void a_func(long v = " << v << ")" << std::endl;
  }

  int another_func()
  {
    std::cout << "a_class::another_func() = 3" << std::endl;
    return 3;
  } 
};

struct another_class
{
  // note: overloaded a_func implemented as a function template
  template<typename T>
  void a_func(T v) const
  {
    std::cout << 
      "another_class::void a_func(T v = " << v << ")" 
      "  [ T = " << typeid(T).name() << " ]" << std::endl;
  }

  int another_func()
  {
    std::cout << "another_class::another_func() = 5" << std::endl;
    return 5;
  } 
};


// both classes above can be assigned to the interface variable and their
// member functions can be called through it
int main()
{
  a_class x;
  another_class y;

  interface_x i(x);
  i.a_func(12);
  i.a_func(77L);
  i.another_func();

  i = y;
  i.a_func(13);
  i.a_func(21L);
  i.another_func();
}

