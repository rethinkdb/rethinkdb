//  Copyright (c) 2000
//  Cadenza New Zealand Ltd
//
//  (C) Copyright John Maddock 2001. 
//
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.

//  MACRO:         BOOST_NO_POINTER_TO_MEMBER_CONST
//  TITLE:         pointers to const member functions
//  DESCRIPTION:   The compiler does not correctly handle
//                 pointers to const member functions, preventing use
//                 of these in overloaded function templates.
//                 See boost/functional.hpp for example.

#include <functional>

namespace boost_no_pointer_to_member_const{

template <class S, class T>
class const_mem_fun_t : public std::unary_function<const T*, S>
{
public:
  explicit const_mem_fun_t(S (T::*p)() const)
      :
      ptr(p)
  {}
  S operator()(const T* p) const
  {
      return (p->*ptr)();
  }
private:
  S (T::*ptr)() const;        
};

template <class S, class T, class A>
class const_mem_fun1_t : public std::binary_function<const T*, A, S>
{
public:
  explicit const_mem_fun1_t(S (T::*p)(A) const)
      :
      ptr(p)
  {}
  S operator()(const T* p, const A& x) const
  {
      return (p->*ptr)(x);
  }
private:
  S (T::*ptr)(A) const;
};

template<class S, class T>
inline const_mem_fun_t<S,T> mem_fun(S (T::*f)() const)
{
  return const_mem_fun_t<S,T>(f);
}

template<class S, class T, class A>
inline const_mem_fun1_t<S,T,A> mem_fun(S (T::*f)(A) const)
{
  return const_mem_fun1_t<S,T,A>(f);
}

class tester
{
public:
   void foo1()const{}
   int foo2(int i)const{ return i*2; }
};


int test()
{
   boost_no_pointer_to_member_const::mem_fun(&tester::foo1);
   boost_no_pointer_to_member_const::mem_fun(&tester::foo2);
   return 0;
}

}




