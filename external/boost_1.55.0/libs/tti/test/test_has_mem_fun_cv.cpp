
//  (C) Copyright Edward Diener 2012
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_mem_fun.hpp"
#include <boost/detail/lightweight_test.hpp>

int main()
  {
  
  // Use const enclosing type
  
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(AConstFunction)<const AType,double,boost::mpl::vector<long,char> >::value));
  BOOST_TEST((StillTest<const AnotherType,AType,boost::mpl::vector<int> >::value));
  
  // Use const_qualified
  
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(AnotherConstFunction)<AnotherType,int,boost::mpl::vector<AType *, short>,boost::function_types::const_qualified>::value));
  BOOST_TEST((AskIfConst<AType,void,boost::mpl::vector<float,double>,boost::function_types::const_qualified>::value));
  
  // Use volatile enclosing type
  
  BOOST_TEST((AnVol<volatile AnotherType,int,boost::mpl::vector<AType *,short> >::value));
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(AVolatileFunction)<volatile AType,double,boost::mpl::vector<long,char> >::value));
  
  // Use volatile_qualified
  
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(StillVolatile)<AnotherType,bool,boost::mpl::vector<int>,boost::function_types::volatile_qualified>::value));
  BOOST_TEST((Volly<AType,void,boost::mpl::vector<float,double>,boost::function_types::volatile_qualified>::value));
  
  // Use const volatile enclosing type
  
  BOOST_TEST((CVAnother<const volatile AnotherType,int,boost::mpl::vector<AType *,short> >::value));
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(StillCV)<const volatile AnotherType,short,boost::mpl::vector<int> >::value));
  
  // Use cv_qualified
  
  BOOST_TEST((BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(ConstVolFunction)<AType,void,boost::mpl::vector<float,double>,boost::function_types::cv_qualified>::value));
  BOOST_TEST((CVBoth<AType,double,boost::mpl::vector<long,char>,boost::function_types::cv_qualified>::value));
  
  return boost::report_errors();

  }
