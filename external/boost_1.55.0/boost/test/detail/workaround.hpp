//  (C) Copyright Gennadiy Rozental 2005-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 54633 $
//
//  Description : contains mics. workarounds 
// ***************************************************************************

#ifndef BOOST_TEST_WORKAROUND_HPP_021005GER
#define BOOST_TEST_WORKAROUND_HPP_021005GER

// Boost
#include <boost/config.hpp> // compilers workarounds and std::ptrdiff_t

// STL
#include <iterator>     // for std::distance

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {

namespace unit_test {

namespace ut_detail {

#ifdef BOOST_NO_STD_DISTANCE
template <class T>
std::ptrdiff_t distance( T const& x_, T const& y_ )
{ 
    std::ptrdiff_t res = 0;

    std::distance( x_, y_, res );

    return res;
}

//____________________________________________________________________________//

#else
using std::distance;
#endif

template <class T> inline void ignore_unused_variable_warning(const T&) {}

} // namespace ut_detail

} // namespace unit_test

namespace unit_test_framework = unit_test;

} // namespace boost

//____________________________________________________________________________//

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_WORKAROUND_HPP_021005GER
