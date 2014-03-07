
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"

boost::unit_test::test_suite* get_master_unit(const char* name)
{
   static boost::unit_test::test_suite* ptest_suite = 0;
   if(0 == ptest_suite)
      ptest_suite = BOOST_TEST_SUITE( name ? name : "" );
   return ptest_suite;
}

boost::unit_test::test_suite* init_unit_test_suite ( int , char* [] )
{
   return get_master_unit();
}



