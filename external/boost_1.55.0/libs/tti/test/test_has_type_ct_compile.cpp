
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_has_type.hpp"
#include <boost/mpl/assert.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/type_traits/is_same.hpp>
using namespace boost::mpl::placeholders;

int main()
  {
  
  // You can always instantiate without compiler errors
  
  TheInteger<AType::BType,boost::is_same<short,_> > aVar;
  BOOST_TTI_HAS_TYPE_GEN(NoOtherType)<AnotherType,boost::is_same<double,_> > aVar2;
  
  // Compile time asserts
  
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TYPE_GEN(AnIntType)<AType,boost::is_same<int,_> >));
  BOOST_MPL_ASSERT((NameStruct<AType,boost::is_same<AType::AStructType,_> >));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TYPE_GEN(AnIntTypeReference)<AType,boost::is_same<int &,_> >));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TYPE_GEN(BType)<AType,boost::is_same<AType::BType,_> >));
  BOOST_MPL_ASSERT((TheInteger<AType::BType,boost::is_same<int,_> >));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TYPE_GEN(CType)<AType::BType,boost::is_same<AType::BType::CType,_> >));
  BOOST_MPL_ASSERT((BOOST_TTI_HAS_TYPE_GEN(AnotherIntegerType)<AType::BType::CType,boost::is_same<int,_> >));
  BOOST_MPL_ASSERT((SomethingElse<AnotherType,boost::is_same<AType::AnIntType,_> >));
  
  return 0;

  }
