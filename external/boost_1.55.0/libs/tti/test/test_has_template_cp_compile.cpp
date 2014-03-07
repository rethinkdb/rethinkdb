
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_template_cp.hpp"
#include <boost/mpl/assert.hpp>

int main()
  {
  
  // You can always instantiate without compiler errors
  
  BOOST_TTI_HAS_TEMPLATE_GEN(TemplateNotExist)<AnotherType> aVar1;
  
  // Compile time asserts
  
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TEMPLATE_GEN(ATPMemberTemplate)<AType>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TEMPLATE_GEN(AMemberTemplate)<AType>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TEMPLATE_GEN(SomeMemberTemplate)<AnotherType>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TEMPLATE_GEN(SimpleTMP)<AnotherType>));

  BOOST_MPL_ASSERT((HaveCL<AType>));
  BOOST_MPL_ASSERT((HaveAnotherMT<AType>));
  BOOST_MPL_ASSERT((ATemplateWithParms<AnotherType>));
  
  return 0;

  }
