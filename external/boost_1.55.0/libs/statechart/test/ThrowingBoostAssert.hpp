#ifndef BOOST_STATECHART_TEST_THROWING_BOOST_ASSERT_HPP_INCLUDED
#define BOOST_STATECHART_TEST_THROWING_BOOST_ASSERT_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <string>
#include <stdexcept>



#define BOOST_ENABLE_ASSERT_HANDLER



namespace boost
{


void assertion_failed(
  char const * expr, char const * func, char const * file, long )
{
  throw std::logic_error(
    std::string( "\nAssertion failed: \"" ) + expr + "\"\n" +
    "File: \"" + file + "\"\n" +
    "Function: \"" + func + "\"\n" );
}



} // namespace boost



#endif
