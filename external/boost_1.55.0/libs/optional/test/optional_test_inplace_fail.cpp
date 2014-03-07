// Copyright (C) 2003, Fernando Luis Cacciola Carballal.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/lib/optional for documentation.
//
// You are welcome to contact the author at:
//  fernando_cacciola@hotmail.com
//
#include<iostream>
#include<stdexcept>
#include<string>

#define BOOST_ENABLE_ASSERT_HANDLER

#include "boost/optional/optional.hpp"

#ifndef BOOST_OPTIONAL_NO_INPLACE_FACTORY_SUPPORT
#include "boost/utility/in_place_factory.hpp"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "boost/test/minimal.hpp"

#include "optional_test_common.cpp"

#ifndef BOOST_OPTIONAL_NO_INPLACE_FACTORY_SUPPORT
struct A
{
  A ( double a0, std::string a1 ) : m_a0(a0), m_a1(a1) {}

  friend bool operator == ( A const& x, A const& y )
    { return x.m_a0 == y.m_a0 && x.m_a1 == y.m_a1 ; }

  double      m_a0 ;
  std::string m_a1 ;
} ;

int test_main( int, char* [] )
{
  int invalid_extra_parameter ;
  boost::optional<A> opt2 ( boost::in_place(3.14,"pi",invalid_extra_parameter) ) ;

  return 0;
}
#else
int test_main( int, char* [] )
{
  int invalid_extra_parameter ;
  boost::optional<A> opt2 ( A(3.14,"pi",invalid_extra_parameter) ) ;

  return 0;
}
#endif

