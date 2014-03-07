
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_template.hpp"
#include <boost/mpl/assert.hpp>

int main()
  {
  
  // You can always instantiate without compiler errors
  
  BOOST_TTI_HAS_TEMPLATE_GEN(AMemberTemplate)<AnotherType> aVar;
  HaveAnotherMT<AnotherType> aVar2;
  BOOST_TTI_HAS_TEMPLATE_GEN(SomeMemberTemplate)<AType> aVar3;
  BOOST_TTI_HAS_TEMPLATE_GEN(TemplateNotExist)<AType> aVar4;
  
  // Compile time asserts
  
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TEMPLATE_GEN(ATPMemberTemplate)<AType>));
  BOOST_MPL_ASSERT((HaveCL<AType>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TEMPLATE_GEN(SimpleTMP)<AnotherType>));
  
  return 0;

  }
