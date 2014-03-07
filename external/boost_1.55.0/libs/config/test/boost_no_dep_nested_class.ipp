//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_DEPENDENT_NESTED_DERIVATIONS
//  TITLE:         dependent nested template classes
//  DESCRIPTION:   The compiler fails to compile
//                 a nested class that has a dependent base class:
//                 template<typename T>
//                 struct foo : {
//                    template<typename U>
//                    struct bar : public U {};
//                 };
#ifndef BOOST_NESTED_TEMPLATE
#define BOOST_NESTED_TEMPLATE template
#endif


namespace boost_no_dependent_nested_derivations{

struct UDT1{};
struct UDT2{};

template<typename T> 
struct foo 
{
  template<typename U> 
  struct bar : public foo<U> 
  {};
};

template <class T>
void foo_test(T)
{
   typedef foo<T> foo_type;
   typedef typename foo_type::BOOST_NESTED_TEMPLATE bar<UDT2> bar_type;
   foo<T> ft;
   bar_type bt;
   (void) &bt;
   (void) &ft;
}

int test()
{
   foo_test(UDT1());
   return 0;
}

}






