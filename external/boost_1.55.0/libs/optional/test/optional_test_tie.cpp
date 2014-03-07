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

#include "boost/optional.hpp"
#include "boost/tuple/tuple.hpp"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "boost/test/minimal.hpp"

#include "optional_test_common.cpp"

// Test boost::tie() interoperabiliy.
int test_main( int, char* [] )
{
  typedef X T ;
          
  try
  {
    TRACE( std::endl << BOOST_CURRENT_FUNCTION  );
  
    T z(0);
    T a(1);
    T b(2);
  
    optional<T> oa, ob ;
  
    // T::T( T const& x ) is used
    set_pending_dtor( ARG(T) ) ;
    set_pending_copy( ARG(T) ) ;
    boost::tie(oa,ob) = std::make_pair(a,b) ;
    check_is_not_pending_dtor( ARG(T) ) ;
    check_is_not_pending_copy( ARG(T) ) ;
    check_initialized(oa);
    check_initialized(ob);
    check_value(oa,a,z);
    check_value(ob,b,z);
    
  }
  catch ( ... )
  {
    BOOST_ERROR("Unexpected Exception caught!");
  }

  return 0;
}


