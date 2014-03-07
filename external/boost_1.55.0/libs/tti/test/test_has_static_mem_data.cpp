
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_static_mem_data.hpp"
#include <boost/detail/lightweight_test.hpp>

int main()
  {
  
  BOOST_TEST((BOOST_TTI_HAS_STATIC_MEMBER_DATA_GEN(DSMember)<AType,short>::value));
  BOOST_TEST((!BOOST_TTI_HAS_STATIC_MEMBER_DATA_GEN(SomeStaticData)<AnotherType,float>::value));
  BOOST_TEST((StatName<AnotherType,AType::AStructType>::value));
  
  BOOST_TEST((BOOST_TTI_HAS_STATIC_MEMBER_DATA_GEN(CIntValue)<AnotherType,const int>::value));
  
  return boost::report_errors();

  }
