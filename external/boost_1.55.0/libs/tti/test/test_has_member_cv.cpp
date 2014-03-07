
//  (C) Copyright Edward Diener 2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_mem_fun.hpp"
#include <boost/detail/lightweight_test.hpp>

int main()
  {
  
  // Use const
  
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(AConstFunction)<double (AType::*)(long,char) const>::value));
  BOOST_TEST((StillTest<AType (AnotherType::*)(int) const>::value));
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(AnotherConstFunction)<int (AnotherType::*)(AType *, short) const>::value));
  BOOST_TEST((AskIfConst<void (AType::*)(float,double) const>::value));
  
  // Use volatile
  
  BOOST_TEST((AnVol<int (AnotherType::*)(AType *,short) volatile>::value));
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(AVolatileFunction)<double (AType::*)(long,char) volatile>::value));
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(StillVolatile)<bool (AnotherType::*)(int) volatile>::value));
  BOOST_TEST((Volly<void (AType::*)(float,double) volatile>::value));
  
  // Use const volatile
  
  BOOST_TEST((CVAnother<int (AnotherType::*)(AType *,short) const volatile>::value));
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(StillCV)<short (AnotherType::*)(int) const volatile>::value));
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(ConstVolFunction)<void (AType::*)(float,double) const volatile>::value));
  BOOST_TEST((CVBoth<double (AType::*)(long,char) const volatile>::value));
  
  return boost::report_errors();

  }
