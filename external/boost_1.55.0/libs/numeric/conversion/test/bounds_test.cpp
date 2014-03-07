//  (c) Copyright Fernando Luis Cacciola Carballal 2000-2004
//  Use, modification, and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See library home page at http://www.boost.org/libs/numeric/conversion
//
// Contact the author at: fernando_cacciola@hotmail.com
// 
#include<typeinfo>
#include<iostream>
#include<iomanip>    

#include "boost/numeric/conversion/bounds.hpp"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "test_helpers.cpp"

using namespace std ;
using namespace boost ;
using namespace numeric ;

// Test the fields of boost::numeric::bounds<> against the expected values.
//
template<class T>
void test_bounds( T expected_lowest, T expected_highest, T expected_smallest )
{
  T lowest   = bounds<T>::lowest  () ;
  T highest  = bounds<T>::highest () ;
  T smallest = bounds<T>::smallest() ;

  BOOST_CHECK_MESSAGE ( lowest == expected_lowest,
                        "bounds<" << typeid(T).name() << ">::lowest() = " << printable(lowest) << ". Expected " << printable(expected_lowest)
                      ) ;

  BOOST_CHECK_MESSAGE ( highest == expected_highest,
                        "bounds<" << typeid(T).name() << ">::highest() = " << printable(highest) << ". Expected " << printable(expected_highest)
                      ) ;

  BOOST_CHECK_MESSAGE ( smallest == expected_smallest,
                        "bounds<" << typeid(T).name() << ">::smallest() = " << printable(smallest) << ". Expected " << printable(expected_smallest)
                      ) ;
}


template<class T>
void test_bounds_integer( MATCH_FNTPL_ARG(T) )
{
  test_bounds(  numeric_limits<T>::min BOOST_PREVENT_MACRO_SUBSTITUTION()
              , numeric_limits<T>::max BOOST_PREVENT_MACRO_SUBSTITUTION()
              , static_cast<T>(1)
             ) ;
}
template<class T>
void test_bounds_float( MATCH_FNTPL_ARG(T))
{
  test_bounds(  -numeric_limits<T>::max BOOST_PREVENT_MACRO_SUBSTITUTION ()
               , numeric_limits<T>::max BOOST_PREVENT_MACRO_SUBSTITUTION ()
               , numeric_limits<T>::min BOOST_PREVENT_MACRO_SUBSTITUTION ()
             ) ;
}

void test_bounds_integers()
{
  test_bounds_integer( SET_FNTPL_ARG(unsigned char) ) ;
  test_bounds_integer( SET_FNTPL_ARG(signed char) ) ;
  test_bounds_integer( SET_FNTPL_ARG(char) ) ;
  test_bounds_integer( SET_FNTPL_ARG(unsigned short) ) ;
  test_bounds_integer( SET_FNTPL_ARG(short) ) ;
  test_bounds_integer( SET_FNTPL_ARG(unsigned int) ) ;
  test_bounds_integer( SET_FNTPL_ARG(int) ) ;
  test_bounds_integer( SET_FNTPL_ARG(unsigned long) ) ;
  test_bounds_integer( SET_FNTPL_ARG(long) ) ;
}

void test_bounds_floats()
{
  test_bounds_float( SET_FNTPL_ARG(float) );
  test_bounds_float( SET_FNTPL_ARG(double) );
  test_bounds_float( SET_FNTPL_ARG(long double) );
}

void test_bounds()
{
  test_bounds_integers() ;
  test_bounds_floats  () ;
}


int test_main( int, char * [] )
{
  cout << setprecision( std::numeric_limits<long double>::digits10 ) ;

  test_bounds();

  return 0;
}

