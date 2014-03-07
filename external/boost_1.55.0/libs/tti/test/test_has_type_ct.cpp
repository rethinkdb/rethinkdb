
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_type.hpp"
#include <boost/detail/lightweight_test.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/type_traits/is_same.hpp>
using namespace boost::mpl::placeholders;

int main()
  {
  
  BOOST_TEST((BOOST_TTI_HAS_TYPE_GEN(AnIntType)<AType,boost::is_same<int,_> >::value));
  BOOST_TEST((NameStruct<AType,boost::is_same<AType::AStructType,_> >::value));
  BOOST_TEST((BOOST_TTI_HAS_TYPE_GEN(AnIntTypeReference)<AType,boost::is_same<int &,_> >::value));
  BOOST_TEST((BOOST_TTI_HAS_TYPE_GEN(BType)<AType,boost::is_same<AType::BType,_> >::value));
  BOOST_TEST((TheInteger<AType::BType,boost::is_same<int,_> >::value));
  BOOST_TEST((BOOST_TTI_HAS_TYPE_GEN(CType)<AType::BType,boost::is_same<AType::BType::CType,_> >::value));
  BOOST_TEST((BOOST_TTI_HAS_TYPE_GEN(AnotherIntegerType)<AType::BType::CType,boost::is_same<int,_> >::value));
  BOOST_TEST((SomethingElse<AnotherType,boost::is_same<AType::AnIntType,_> >::value));
  BOOST_TEST((!BOOST_TTI_HAS_TYPE_GEN(NoOtherType)<AnotherType,boost::is_same<double,_> >::value));
  
  return boost::report_errors();

  }
