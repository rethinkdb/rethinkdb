
//  (C) Copyright Edward Diener 2012
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_data.hpp"
#include <boost/mpl/assert.hpp>

int main()
  {
  
  // You can always instantiate without compiler errors
  
  BOOST_TTI_HAS_DATA_GEN(aMember)<AType,long> aVar;
  BOOST_TTI_HAS_DATA_GEN(SomeStaticData)<AnotherType,long> aVar2;
  BOOST_TTI_HAS_DATA_GEN(someDataMember)<AnotherType,double> aVar3;
  
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_DATA_GEN(AnInt)<AType,int>));
  BOOST_MPL_ASSERT_NOT((BOOST_TTI_HAS_DATA_GEN(SomeStaticData)<AnotherType,float>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_DATA_GEN(AnInt)<AnotherType,long>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_DATA_GEN(DSMember)<AType,short>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_DATA_GEN(aMember)<AnotherType,bool>));
  BOOST_MPL_ASSERT_NOT((BOOST_TTI_HAS_DATA_GEN(aMember)<AnotherType,int>));
  BOOST_MPL_ASSERT((DCMember<AnotherType,bool>));
  BOOST_MPL_ASSERT_NOT((BOOST_TTI_HAS_DATA_GEN(someDataMember)<AType,short>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_DATA_GEN(IntBT)<AType,AType::BType>));
  BOOST_MPL_ASSERT((DStatName<AnotherType,AType::AStructType>));
  BOOST_MPL_ASSERT((DNestedData<AType,AType::BType::CType>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_DATA_GEN(CIntValue)<AnotherType,const int>));
  BOOST_MPL_ASSERT((DAOther<AnotherType,AType>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_DATA_GEN(ONestStr)<AnotherType,AType::AStructType>));
  
  return 0;
  
  }
