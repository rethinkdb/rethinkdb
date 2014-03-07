
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_type.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/function_traits.hpp>
#endif

typedef void(pf_zero1)();
typedef int(pf_zero2)();
typedef const int& (pf_zero3)();
typedef void(pf_one1)(int);
typedef int(pf_one2)(int);
typedef const int&(pf_one3)(const int&);
typedef void(pf_two1)(int,int);
typedef int(pf_two2)(int,int);
typedef const int&(pf_two3)(const int&,const int&);


TT_TEST_BEGIN(function_traits)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::function_traits<pf_zero1>::arity, 0);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::function_traits<pf_zero2>::arity, 0);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::function_traits<pf_zero3>::arity, 0);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::function_traits<pf_one1>::arity, 1);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::function_traits<pf_one2>::arity, 1);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::function_traits<pf_one3>::arity, 1);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::function_traits<pf_two1>::arity, 2);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::function_traits<pf_two2>::arity, 2);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::function_traits<pf_two3>::arity, 2);

BOOST_CHECK_TYPE(void, ::tt::function_traits<pf_zero1>::result_type);
BOOST_CHECK_TYPE(::tt::function_traits<pf_zero2>::result_type, int);
BOOST_CHECK_TYPE(::tt::function_traits<pf_zero3>::result_type, const int&);
BOOST_CHECK_TYPE(::tt::function_traits<pf_one1>::result_type, void);
BOOST_CHECK_TYPE(::tt::function_traits<pf_one2>::result_type, int);
BOOST_CHECK_TYPE(::tt::function_traits<pf_one3>::result_type, const int&);
BOOST_CHECK_TYPE(::tt::function_traits<pf_two1>::result_type, void);
BOOST_CHECK_TYPE(::tt::function_traits<pf_two2>::result_type, int);
BOOST_CHECK_TYPE(::tt::function_traits<pf_two3>::result_type, const int&);

BOOST_CHECK_TYPE(::tt::function_traits<pf_one1>::arg1_type, int);
BOOST_CHECK_TYPE(::tt::function_traits<pf_one2>::arg1_type, int);
BOOST_CHECK_TYPE(::tt::function_traits<pf_one3>::arg1_type, const int&);

BOOST_CHECK_TYPE(::tt::function_traits<pf_two1>::arg1_type, int);
BOOST_CHECK_TYPE(::tt::function_traits<pf_two2>::arg1_type, int);
BOOST_CHECK_TYPE(::tt::function_traits<pf_two3>::arg1_type, const int&);
BOOST_CHECK_TYPE(::tt::function_traits<pf_two1>::arg2_type, int);
BOOST_CHECK_TYPE(::tt::function_traits<pf_two2>::arg2_type, int);
BOOST_CHECK_TYPE(::tt::function_traits<pf_two3>::arg2_type, const int&);

TT_TEST_END








