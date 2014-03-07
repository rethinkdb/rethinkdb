
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_convertible.hpp>
#endif
#include <boost/utility/enable_if.hpp>


template <class T>
struct convertible_from
{
    convertible_from(T);
};

struct base2 { };
struct middle2 : public virtual base2 { };
struct derived2 : public middle2 { };

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

template<typename T>
struct test_bug_4530
{
	template<typename A>
	test_bug_4530(A&&, typename boost::enable_if<boost::is_convertible<A&&, T> >::type* =0);
};

struct A4530
{
   template <class T>
   A4530(T);
};

#endif

#ifdef BOOST_MSVC

struct bug_5271a
{
    __declspec(align(16)) int value;
};

struct bug_5271b
{
    __declspec(align(16)) int value;
    bug_5271b(int v) : value(v) {}
};

#endif

TT_TEST_BEGIN(is_convertible)

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<Derived,Base>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<Derived,Derived>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<Base,Base>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<Base,Derived>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<Derived,Derived>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<NonDerived,Base>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<float,int>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<virtual_inherit2,virtual_inherit1>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<VD,VB>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<polymorphic_derived1,polymorphic_base>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<polymorphic_derived2,polymorphic_base>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<polymorphic_base,polymorphic_derived1>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<polymorphic_base,polymorphic_derived2>::value), false);
#ifndef TEST_STD
// Ill-formed behaviour, supported by Boost as an extension:
#ifndef BOOST_NO_IS_ABSTRACT
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<test_abc1,test_abc1>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<Base,test_abc1>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<polymorphic_derived2,test_abc1>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<int,test_abc1>::value), false);
#endif
#endif

// The following four do not compile without member template support:
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<float,void>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<void,void>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<void,float>::value), false);

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<enum1, int>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<Derived*, Base*>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<Base*, Derived*>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<Derived&, Base&>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<Base&, Derived&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<const Derived*, const Base*>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<const Base*, const Derived*>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<const Derived&, const Base&>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<const Base&, const Derived&>::value), false);

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<const int *, int*>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<const int&, int&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<const int*, int[3]>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<const int&, int>::value), true);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<const int&&, int>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<int&&, const int&>::value), true);
#if !defined(__GNUC__) || ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 6)))
// Machinary required to support this, not available prior to gcc-4.7.0:
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<int, int&>::value), false);
#endif
#endif
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<int(&)[4], const int*>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<int(&)(int), int(*)(int)>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<int *, const int*>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<int&, const int&>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<int[2], int*>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<int[2], const int*>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<const int[2], int*>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<int*, int[3]>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<test_abc3, const test_abc1&>::value), true);

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<non_pointer, void*>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<non_pointer, int*>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<non_int_pointer, int*>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<non_int_pointer, void*>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<test_abc1&, test_abc2&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<test_abc1&, int_constructible>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<int_constructible, test_abc1&>::value), false);
#if !defined(BOOST_NO_IS_ABSTRACT) && !(defined(__hppa) && defined(__HP_aCC))
//
// This doesn't work with aCC on PA RISC even though the is_abstract tests do
// all pass, this may indicate a deeper problem...
//
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<test_abc1&, test_abc2>::value), false);
#endif

//
// the following tests all involve user defined conversions which do
// not compile with Borland C++ Builder 5:
//
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<int, int_constructible>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<float,convertible_from<float> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<float,convertible_from<float const&> >::value), true);
//
// These two tests give different results with different compilers, we used to require *true* results
// from these, but C++0x behaviour requires *false*:
//
//BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<float,convertible_from<float&> >::value), false);
//BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<char,convertible_from<char&> >::value), false);

#if !(defined(__GNUC__) && (__GNUC__ < 4))
// GCC 3.x emits warnings here, which causes the tests to fail when we compile with warnings-as-errors:
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<float,convertible_from<char> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<float,convertible_from<char const&> >::value), true);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<float,convertible_from<char&> >::value), false);

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<char,convertible_from<char> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<char,convertible_from<char const&> >::value), true);

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<float&,convertible_from<float> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<float const&,convertible_from<float> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<float&,convertible_from<float&> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<float const&,convertible_from<float const&> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<float&,convertible_from<float const&> >::value), true);

//
// the following all generate warnings unless we can find a way to
// suppress them:
//
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<float,int>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<double,int>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<double,float>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<long,int>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<int,char>::value), true);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<boost::long_long_type,int>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<boost::long_long_type,char>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<boost::long_long_type,float>::value), true);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<__int64,int>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<__int64,char>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<__int64,float>::value), true);
#endif

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
// Test bug case 4530:
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<test_bug_4530<A4530>,A4530>::value), true);
#endif

#if defined(BOOST_MSVC) && (BOOST_MSVC > 1310)
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<int, bug_5271a>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<int, bug_5271b>::value), true);
#endif

TT_TEST_END

