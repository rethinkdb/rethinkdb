
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_type.hpp"
#include <boost/mpl/assert.hpp>

int main()
  {
  
  // You can always instantiate without compiler errors
  
  BOOST_TTI_HAS_TYPE_GEN(AnIntType)<AnotherType> aVar1;
  BOOST_TTI_HAS_TYPE_GEN(NoOtherType)<AType> aVar2;
  
  // Compile time asserts
  
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TYPE_GEN(AnIntType)<AType>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TYPE_GEN(AnIntTypeReference)<AType>));
  BOOST_MPL_ASSERT((NameStruct<AType>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TYPE_GEN(BType)<AType>));
  BOOST_MPL_ASSERT((TheInteger<AType::BType>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TYPE_GEN(CType)<AType::BType>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TYPE_GEN(AnotherIntegerType)<AType::BType::CType>));
  BOOST_MPL_ASSERT((SomethingElse<AnotherType>));
  
  return 0;

  }
