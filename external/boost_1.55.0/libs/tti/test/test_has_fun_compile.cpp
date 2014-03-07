
//  (C) Copyright Edward Diener 2012
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_fun.hpp"
#include <boost/mpl/assert.hpp>

int main()
  {
  
  // You can always instantiate without compiler errors
  
  TheTIntFunction<AType,void,boost::mpl::vector<long,double> > aVar;
  BOOST_TTI_HAS_FUNCTION_GEN(someFunctionMember)<AnotherType,double,boost::mpl::vector<short,short,long,int> > aVar2;
  Pickedname<AnotherType,AType,boost::mpl::vector<long,long> > aVar3;
  
  // Compile time asserts
  
  BOOST_MPL_ASSERT((TheTIntFunction<AnotherType,AType,boost::mpl::vector<long,double> >));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_FUNCTION_GEN(VoidFunction)<AType,void>));
  BOOST_MPL_ASSERT((FunctionReturningInt<AType,int>));
  BOOST_MPL_ASSERT((FunctionReturningInt<AnotherType,double,boost::mpl::vector<int> >));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_FUNCTION_GEN(TSFunction)<AnotherType,AType::AStructType,boost::mpl::vector<AType::AnIntType,double> >));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_FUNCTION_GEN(aFunction)<AnotherType,AType,boost::mpl::vector<int> >));
  BOOST_MPL_ASSERT((AnotherIntFunction<AnotherType,int,boost::mpl::vector<AType> >));
  BOOST_MPL_ASSERT((HaveTheSIntFunction<AType,int,boost::mpl::vector<long,double> >));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_FUNCTION_GEN(sFunction)<AnotherType,AType::AnIntType,boost::mpl::vector<int,long,double> >));
  
  return 0;

  }
