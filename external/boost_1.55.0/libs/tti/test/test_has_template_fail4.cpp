
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_template.hpp"
#include <boost/mpl/assert.hpp>

int main()
  {
  
  // Not all 'typename' parameters
  
  BOOST_MPL_ASSERT((HaveAnotherMT<AType>));
  
  return 0;

  }
