
//  (C) Copyright Edward Diener 2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_mem_fun.hpp"
#include <boost/mpl/assert.hpp>

int main()
  {
  
  // With default parameters you must still specify complete member function prototype
  
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(AVolatileFunction)<double (AType::*)(long) volatile>));
  
  return 0;

  }
