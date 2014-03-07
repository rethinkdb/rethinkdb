
//  (C) Copyright John Maddock 2005. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_base_of.hpp>
#endif



TT_TEST_BEGIN(is_base_of)

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Derived,Base>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Derived,Derived>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Derived,const Derived>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Base,Base>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Base,Derived>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<const Base,Derived>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Base,const Derived>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Base,MultiBase>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Derived,MultiBase>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Derived2,MultiBase>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Base,PrivateBase>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<NonDerived,Base>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Base,void>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Base,const void>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<void,Derived>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<const void,Derived>::value), false);
#if defined(TEST_STD) && (TEST_STD < 2006)
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<int, int>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<const int, int>::value), true);
#else
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<int, int>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<const int, int>::value), false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<VB,VD>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<VD,VB>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<test_abc1,test_abc3>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<test_abc3,test_abc1>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Base,virtual_inherit1>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<virtual_inherit1,Base>::value), false);

TT_TEST_END









