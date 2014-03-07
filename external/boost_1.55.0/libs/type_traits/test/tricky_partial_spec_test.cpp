
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#include <boost/type_traits/alignment_of.hpp>
#include <boost/type_traits/has_nothrow_assign.hpp>
#include <boost/type_traits/has_nothrow_constructor.hpp>
#include <boost/type_traits/has_nothrow_copy.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_polymorphic.hpp>
#include <boost/type_traits/is_member_pointer.hpp>
#include <boost/type_traits/is_member_object_pointer.hpp>
#include <boost/type_traits/is_member_function_pointer.hpp>
#include <boost/type_traits/is_pointer.hpp>
#endif
#include <stdexcept>
#include <new>
#include <exception>

//
// VC++ emits an awful lot of warnings unless we define these:
#ifdef BOOST_MSVC
#  pragma warning(disable:4244)
#endif


template <class T>
struct align_calc
{
   char padding;
   T instance;
   static std::ptrdiff_t get()
   {
      static align_calc<T> a;
      return reinterpret_cast<const char*>(&(a.instance)) - reinterpret_cast<const char*>(&(a.padding));
   }
};

#define ALIGNOF(x) align_calc<x>::get()


TT_TEST_BEGIN(tricky_partial_specialization_test)
//
// corner cases which don't compile without partial specialization
// support:
//

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<char&>::value, ALIGNOF(void*));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<char (&)(int)>::value, ALIGNOF(void*));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<char(&)[4]>::value, ALIGNOF(void*));

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Base&,Derived>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Base&,Derived&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Base,Derived&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Base,void>::value), false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Base&&,Derived>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Base&&,Derived&&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_base_of<Base,Derived&&>::value), false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<void, int>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<void, void>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<void, const void>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<test_abc1, test_abc1>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<test_abc1, const test_abc1>::value), false);

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_convertible<void,float>::value), false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<const UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<volatile empty_UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<const VB>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<const volatile VD>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<const test_abc1>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<volatile test_abc2>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<const std::exception>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<const std::bad_alloc>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<const std::runtime_error>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<const std::out_of_range>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<const std::range_error>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pointer<const f1>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pointer<volatile f1>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pointer<const volatile f1>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<const f1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<const mp>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<volatile mp>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<const volatile mp>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<const mf3>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<volatile mf3>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<const volatile mf3>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<const f1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<const mp>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<volatile mp>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<const volatile mp>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<const mf3>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<volatile mf3>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<const volatile mf3>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_function_pointer<const f1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_function_pointer<const mp>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_function_pointer<volatile mp>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_function_pointer<const volatile mp>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_function_pointer<const mf3>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_function_pointer<volatile mf3>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_function_pointer<const volatile mf3>::value, true);

TT_TEST_END








