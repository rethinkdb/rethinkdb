
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_base_and_derived.hpp>
#endif

//
// Additional tests added for VC7.1 bug, 2005/04/21
//
struct marker{};
struct foo{ int x; };

template<class Class,typename Type,Type Class::*PtrToMember>
struct class_member{};
template<class Class,typename Type,Type Class::*PtrToMember>
struct class_member2 : public marker{};


TT_TEST_BEGIN(is_base_and_derived)

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<Derived,Base>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<Derived,Derived>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<Base,Base>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<const Base,Base>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<Base,Derived>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<Base,MultiBase>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<Derived,MultiBase>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<Derived2,MultiBase>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<Base,PrivateBase>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<NonDerived,Base>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<Base,void>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<Base,const void>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<void,Derived>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<const void,Derived>::value), false);
#if defined(TEST_STD) && (TEST_STD < 2006)
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<int, int>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<const int, int>::value), true);
#else
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<int, int>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<const int, int>::value), false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<VB,VD>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<VD,VB>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<test_abc1,test_abc3>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<test_abc3,test_abc1>::value), false);

typedef class_member<foo,int,&foo::x> mem_type;
typedef class_member2<foo,int,&foo::x> mem2_type;
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<marker, mem_type>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_and_derived<marker, mem2_type>::value), true);

TT_TEST_END









