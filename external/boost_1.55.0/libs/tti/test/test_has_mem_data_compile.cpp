
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_mem_data.hpp"
#include <boost/mpl/assert.hpp>

int main()
  {
  
  // You can always instantiate without compiler errors
  
  BOOST_TTI_HAS_MEMBER_DATA_GEN(aMember)<AType,long> aVar;
  BOOST_TTI_HAS_MEMBER_DATA_GEN(someDataMember)<AnotherType,double> aVar2;
  
  // Compile time asserts
  
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_DATA_GEN(AnInt)<AType,int>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_DATA_GEN(AnInt)<AnotherType,long>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_DATA_GEN(aMember)<AnotherType,bool>));
  BOOST_MPL_ASSERT((CMember<AnotherType,bool>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_DATA_GEN(IntBT)<AType,AType::BType>));
  BOOST_MPL_ASSERT((NestedData<AType,AType::BType::CType>));
  BOOST_MPL_ASSERT((AOther<AnotherType,AType>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_DATA_GEN(ONestStr)<AnotherType,AType::AStructType>));
  
  return 0;

  }
