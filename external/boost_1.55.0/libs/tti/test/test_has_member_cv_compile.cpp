
//  (C) Copyright Edward Diener 2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_mem_fun.hpp"
#include <boost/mpl/assert.hpp>

int main()
  {
  
  // You can always instantiate without compiler errors
  
  BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(someFunctionMember)<double (AnotherType::*)(short,short,long,int) volatile> aVar1;
  
  // Use const
  
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(AConstFunction)<double (AType::*)(long,char) const>));
  BOOST_MPL_ASSERT((StillTest<AType (AnotherType::*)(int) const>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(AnotherConstFunction)<int (AnotherType::*)(AType *, short) const>));
  BOOST_MPL_ASSERT((AskIfConst<void (AType::*)(float,double) const>));
  
  // Use volatile
  
  BOOST_MPL_ASSERT((AnVol<int (AnotherType::*)(AType *,short) volatile>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(AVolatileFunction)<double (AType::*)(long,char) volatile>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(StillVolatile)<bool (AnotherType::*)(int) volatile>));
  BOOST_MPL_ASSERT((Volly<void (AType::*)(float,double) volatile>));
  
  // Use const volatile
  
  BOOST_MPL_ASSERT((CVAnother<int (AnotherType::*)(AType *,short) const volatile>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(StillCV)<short (AnotherType::*)(int) const volatile>));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(ConstVolFunction)<void (AType::*)(float,double) const volatile>));
  BOOST_MPL_ASSERT((CVBoth<double (AType::*)(long,char) const volatile>));
  
  return 0;

  }
