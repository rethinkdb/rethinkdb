//  Copyright (C) 2007 Douglas Gregor
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_CXX11_RVALUE_REFERENCES
//  TITLE:         C++0x rvalue references unavailable
//  DESCRIPTION:   The compiler does not support C++0x rvalue references

namespace boost_no_cxx11_rvalue_references {

void g(int&) {}

template<typename F, typename T>
void forward(F f, T&& t) { f(static_cast<T&&>(t)); }

int test()
{
   int x;
   forward(g, x);
   return 0;
}

}
