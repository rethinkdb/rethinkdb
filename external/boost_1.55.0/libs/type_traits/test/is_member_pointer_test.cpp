
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_member_pointer.hpp>
#endif

TT_TEST_BEGIN(is_member_pointer)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<f1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<f2>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<f3>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<void*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<mf1>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<mf2>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<mf3>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<mf4>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<cmf>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<mp>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<void>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<test_abc1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<foo0_t>::value, false);

#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
typedef void (__stdcall test_abc1::*scall_proc)();
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<scall_proc>::value, true);
typedef void (__fastcall test_abc1::*fcall_proc)(int);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<fcall_proc>::value, true);
typedef void (__cdecl test_abc1::*ccall_proc)(int, long, double);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<ccall_proc>::value, true);
#endif

TT_TEST_END








