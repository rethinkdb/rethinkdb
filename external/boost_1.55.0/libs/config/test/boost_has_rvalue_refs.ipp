//  Copyright (C) 2007 Douglas Gregor
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_RVALUE_REFS
//  TITLE:         rvalue references
//  DESCRIPTION:   The compiler supports C++0x rvalue references

namespace boost_has_rvalue_refs {

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
