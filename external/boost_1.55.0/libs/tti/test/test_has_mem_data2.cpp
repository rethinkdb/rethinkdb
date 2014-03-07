
//  (C) Copyright Edward Diener 2012
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_mem_data.hpp"
#include <boost/detail/lightweight_test.hpp>

int main()
  {
  
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_DATA_GEN(AnInt)<int AType::*>::value));
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_DATA_GEN(AnInt)<long AnotherType::*>::value));
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_DATA_GEN(aMember)<bool AnotherType::*>::value));
  BOOST_TEST((!BOOST_TTI_HAS_MEMBER_DATA_GEN(aMember)<int AnotherType::*>::value));
  BOOST_TEST((CMember<bool AnotherType::*>::value));
  BOOST_TEST((!BOOST_TTI_HAS_MEMBER_DATA_GEN(someDataMember)<short AType::*>::value));
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_DATA_GEN(IntBT)<AType::BType AType::*>::value));
  BOOST_TEST((NestedData<AType::BType::CType AType::*>::value));
  BOOST_TEST((AOther<AType AnotherType::*>::value));
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_DATA_GEN(ONestStr)<AType::AStructType AnotherType::*>::value));
  
  return boost::report_errors();

  }
