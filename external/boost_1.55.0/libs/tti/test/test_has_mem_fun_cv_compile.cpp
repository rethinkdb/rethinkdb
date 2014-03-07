
//  (C) Copyright Edward Diener 2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_mem_fun.hpp"
#include <boost/mpl/assert.hpp>

int main()
  {
  
  // You can always instantiate without compiler errors even if the member function does not exist
  
  BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(someFunctionMember)<AnotherType,double,boost::mpl::vector<short,short,long,int>,boost::function_types::const_qualified> aVar1;
  
  // Use const enclosing type
  
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(AnotherConstFunction)<const AnotherType,int,boost::mpl::vector<AType *, short> >));
  BOOST_MPL_ASSERT((AskIfConst<const AType,void,boost::mpl::vector<float,double> >));
  
  // Use const_qualified
  
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(AConstFunction)<AType,double,boost::mpl::vector<long,char>,boost::function_types::const_qualified>));
  BOOST_MPL_ASSERT((StillTest<AnotherType,AType,boost::mpl::vector<int>,boost::function_types::const_qualified>));
  
  // Use volatile enclosing type
  
  BOOST_MPL_ASSERT((AnVol<volatile AnotherType,int,boost::mpl::vector<AType *,short> >));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(AVolatileFunction)<volatile AType,double,boost::mpl::vector<long,char> >));
  
  // Use volatile_qualified
  
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(StillVolatile)<AnotherType,bool,boost::mpl::vector<int>,boost::function_types::volatile_qualified>));
  BOOST_MPL_ASSERT((Volly<AType,void,boost::mpl::vector<float,double>,boost::function_types::volatile_qualified>));
  
  // Use const volatile enclosing type
  
  BOOST_MPL_ASSERT((CVAnother<const volatile AnotherType,int,boost::mpl::vector<AType *,short> >));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(StillCV)<const volatile AnotherType,short,boost::mpl::vector<int> >));
  
  // Use cv_qualified
  
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(ConstVolFunction)<AType,void,boost::mpl::vector<float,double>,boost::function_types::cv_qualified>));
  BOOST_MPL_ASSERT((CVBoth<AType,double,boost::mpl::vector<long,char>,boost::function_types::cv_qualified>));
  
  return 0;

  }
