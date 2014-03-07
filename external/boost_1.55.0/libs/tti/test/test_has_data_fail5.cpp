
//  (C) Copyright Edward Diener 2012
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_data.hpp"
#include <boost/mpl/assert.hpp>

int main()
  {
  
  // Wrong enclosing type for DSMember
  
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_DATA_GEN(DSMember)<AnotherType,short>));
  
  return 0;

  }
