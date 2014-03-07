
//  (C) Copyright Edward Diener 2012
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_data.hpp"
#include <boost/detail/lightweight_test.hpp>

int main()
  {
  
  BOOST_TEST((BOOST_TTI_HAS_DATA_GEN(AnInt)<AType,int>::value));
  BOOST_TEST((!BOOST_TTI_HAS_DATA_GEN(SomeStaticData)<AnotherType,float>::value));
  BOOST_TEST((BOOST_TTI_HAS_DATA_GEN(AnInt)<AnotherType,long>::value));
  BOOST_TEST((BOOST_TTI_HAS_DATA_GEN(DSMember)<AType,short>::value));
  BOOST_TEST((BOOST_TTI_HAS_DATA_GEN(aMember)<AnotherType,bool>::value));
  BOOST_TEST((!BOOST_TTI_HAS_DATA_GEN(aMember)<AnotherType,int>::value));
  BOOST_TEST((DCMember<AnotherType,bool>::value));
  BOOST_TEST((!BOOST_TTI_HAS_DATA_GEN(someDataMember)<AType,short>::value));
  BOOST_TEST((BOOST_TTI_HAS_DATA_GEN(IntBT)<AType,AType::BType>::value));
  BOOST_TEST((DStatName<AnotherType,AType::AStructType>::value));
  BOOST_TEST((DNestedData<AType,AType::BType::CType>::value));
  BOOST_TEST((BOOST_TTI_HAS_DATA_GEN(CIntValue)<AnotherType,const int>::value));
  BOOST_TEST((DAOther<AnotherType,AType>::value));
  BOOST_TEST((BOOST_TTI_HAS_DATA_GEN(ONestStr)<AnotherType,AType::AStructType>::value));
  
  return boost::report_errors();

  }
