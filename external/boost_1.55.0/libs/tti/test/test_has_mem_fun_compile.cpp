
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_mem_fun.hpp"
#include <boost/mpl/assert.hpp>

int main()
  {
  
  // You can always instantiate without compiler errors
  
  BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(someFunctionMember)<AnotherType,double,boost::mpl::vector<short,short,long,int> > aVar3;
  
  // Compile time asserts
  
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(VoidFunction)<AType,void>));
  BOOST_MPL_ASSERT((FunctionReturningInt<AType,int>));
  BOOST_MPL_ASSERT((FunctionReturningInt<AnotherType,double,boost::mpl::vector<int> >));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(aFunction)<AnotherType,AType,boost::mpl::vector<int> >));
  BOOST_MPL_ASSERT((AnotherIntFunction<AnotherType,int,boost::mpl::vector<AType> >));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(sFunction)<AnotherType,AType::AnIntType,boost::mpl::vector<int,long,double> >));
  
  return 0;

  }
