
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_mem_fun.hpp"
#include <boost/detail/lightweight_test.hpp>

int main()
  {
  
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(VoidFunction)<void (AType::*)()>::value));
  BOOST_TEST(FunctionReturningInt<int (AType::*)()>::value);
  BOOST_TEST(FunctionReturningInt<double (AnotherType::*)(int)>::value);
  BOOST_TEST(BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(aFunction)<AType (AnotherType::*)(int)>::value);
  BOOST_TEST(AnotherIntFunction<int (AnotherType::*)(AType)>::value);
  BOOST_TEST(BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(sFunction)<AType::AnIntType (AnotherType::*)(int,long,double)>::value);
  BOOST_TEST(!BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(someFunctionMember)<AType (AnotherType::*)(long,int)>::value);
  
  return boost::report_errors();

  }
