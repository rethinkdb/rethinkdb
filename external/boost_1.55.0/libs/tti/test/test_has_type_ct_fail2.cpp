
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_type.hpp"
#include <boost/mpl/assert.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/type_traits/is_same.hpp>
using namespace boost::mpl::placeholders;

int main()
  {
  
  // Wrong enclosing type
  
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TYPE_GEN(BType)<AnotherType,boost::is_same<short,_> >));
  
  return 0;

  }
