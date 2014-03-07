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
#include "boost/utility/typed_in_place_factory.hpp"
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
  double a00 = 3.14, a10 = 6.02e-23;
  std::string a01("pi"), a11("mol");

  A a0(a00,a01);
  A a1(a10,a11);

  boost::optional<A> opt1(a0);

  boost::optional<A> opt2 ( boost::in_place(a00,a01) ) ;

  boost::optional<A> opt3 ( boost::in_place<A>(a00,a01) ) ;

  BOOST_CHECK( opt1 == opt2 ) ;
  BOOST_CHECK( opt2 == opt2 ) ;
  BOOST_CHECK( *opt2 == a0 ) ;

#ifndef BOOST_OPTIONAL_WEAK_OVERLOAD_RESOLUTION

  opt2 = boost::in_place(a10,a11);
  BOOST_CHECK( *opt2 == a1 ) ;

  opt3 = boost::in_place<A>(a10,a11);
  BOOST_CHECK( *opt3 == a1 ) ;

#endif

  return 0;
}
#else
int test_main( int, char* [] )
{
  // If in-place factories are not supported there is nothing to test
  return 0 ;
}
#endif



