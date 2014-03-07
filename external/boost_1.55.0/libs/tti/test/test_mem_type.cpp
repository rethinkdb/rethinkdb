
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_mem_type.hpp"
#include <boost/detail/lightweight_test.hpp>

int main()
  {
  
  BOOST_TEST((boost::tti::valid_member_type<BOOST_TTI_MEMBER_TYPE_GEN(AnIntType)<AType>::type>::value));
  BOOST_TEST((boost::tti::valid_member_type<BOOST_TTI_MEMBER_TYPE_GEN(AnIntTypeReference)<AType>::type>::value));
  BOOST_TEST((boost::tti::valid_member_type<BOOST_TTI_MEMBER_TYPE_GEN(BType)<AType>::type>::value));
  BOOST_TEST((boost::tti::valid_member_type<TheInteger<AType::BType>::type>::value));
  BOOST_TEST((boost::tti::valid_member_type<BOOST_TTI_MEMBER_TYPE_GEN(AnotherIntegerType)<AType::BType::CType>::type>::value));
  BOOST_TEST((boost::tti::valid_member_type<SomethingElse<AnotherType>::type>::value));
  
  BOOST_TEST((boost::tti::valid_member_type<NameStruct<AType,MarkerType>::type,NameStruct<AType,MarkerType>::boost_tti_marker_type>::value));
  BOOST_TEST((boost::tti::valid_member_type<BOOST_TTI_MEMBER_TYPE_GEN(CType)<AType::BType,MarkerType>::type,BOOST_TTI_MEMBER_TYPE_GEN(CType)<AType::BType,MarkerType>::boost_tti_marker_type>::value));
  
  BOOST_TEST((!boost::tti::valid_member_type<BOOST_TTI_MEMBER_TYPE_GEN(BType)<AnotherType>::type>::value));
  BOOST_TEST((!boost::tti::valid_member_type<BOOST_TTI_MEMBER_TYPE_GEN(NoExistType)<AType,MarkerType>::type,BOOST_TTI_MEMBER_TYPE_GEN(NoExistType)<AType,MarkerType>::boost_tti_marker_type>::value));
  
  return boost::report_errors();

  }
