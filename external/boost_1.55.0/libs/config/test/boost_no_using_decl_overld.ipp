//  (C) Copyright Eric Friedman 2002.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  MACRO:         BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE
//  TITLE:         using declaration function overloads from a typename base
//  DESCRIPTION:   The compiler will not accept a using declaration
//                 that brings a function from a typename used as a base class
//                 into a derived class if functions of the same name
//                 are present in the derived class.

namespace boost_no_using_declaration_overloads_from_typename_base {

struct base
{
   static void f() { }
};

template <typename T, typename Base>
struct using_overloads_from_typename_base : Base
{
   using Base::f;
   static T f(const T& t) { return t; }
};

int test()
{
   using_overloads_from_typename_base<int,base>::f();
   return using_overloads_from_typename_base<int,base>::f(0);
}

}
