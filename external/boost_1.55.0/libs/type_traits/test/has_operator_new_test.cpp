//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#include <boost/type_traits/has_new_operator.hpp>

#ifdef BOOST_INTEL
//  remark #1720: function "class_with_new_op::operator new" has no corresponding member operator delete (to be called if an exception is thrown during initialization of an allocated object)
//      void * operator new(std::size_t);
//             ^
#pragma warning(disable:1720)
#endif

struct class_with_new_op {
    void * operator new(std::size_t);
};

struct derived_class_with_new_op : public class_with_new_op {};

struct class_with_new_op2 {
   void* operator new(std::size_t size, const std::nothrow_t&);
};

struct class_with_new_op3 {
   void* operator new[](std::size_t size);
};

struct class_with_new_op4 {
   void* operator new[](std::size_t size, const std::nothrow_t&);
};

struct class_with_new_op5 {
   void* operator new (std::size_t size, void* ptr);
};

struct class_with_new_op6 {
   void* operator new[] (std::size_t size, void* ptr);
};

struct class_with_all_ops
{
   void * operator new(std::size_t);
   void* operator new(std::size_t size, const std::nothrow_t&);
   void* operator new[](std::size_t size);
   void* operator new[](std::size_t size, const std::nothrow_t&);
   void* operator new (std::size_t size, void* ptr);
   void* operator new[] (std::size_t size, void* ptr);
};

TT_TEST_BEGIN(has_new_operator)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<class_with_new_op>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<derived_class_with_new_op>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<class_with_new_op2>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<class_with_new_op3>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<class_with_new_op4>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<class_with_new_op5>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<class_with_new_op6>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<class_with_all_ops>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<bool>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<bool const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<bool volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<bool const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<signed char>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<signed char const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<signed char volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<signed char const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned char>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<char>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned char const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<char const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned char volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<char volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned char const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<char const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned short>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<short>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned short const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<short const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned short volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<short volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned short const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<short const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned int const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<int const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned int volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<int volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned int const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<int const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned long>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<long>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned long const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<long const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned long volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<long volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned long const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<long const volatile>::value, false);

#ifdef BOOST_HAS_LONG_LONG

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator< ::boost::ulong_long_type>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator< ::boost::long_long_type>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator< ::boost::ulong_long_type const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator< ::boost::long_long_type const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator< ::boost::ulong_long_type volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator< ::boost::long_long_type volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator< ::boost::ulong_long_type const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator< ::boost::long_long_type const volatile>::value, false);

#endif

#ifdef BOOST_HAS_MS_INT64

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned __int8>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<__int8>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned __int8 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<__int8 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned __int8 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<__int8 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned __int8 const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<__int8 const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned __int16>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<__int16>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned __int16 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<__int16 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned __int16 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<__int16 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned __int16 const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<__int16 const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned __int32>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<__int32>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned __int32 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<__int32 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned __int32 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<__int32 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned __int32 const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<__int32 const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned __int64>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<__int64>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned __int64 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<__int64 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned __int64 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<__int64 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<unsigned __int64 const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<__int64 const volatile>::value, false);

#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<float>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<float const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<float volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<float const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<double>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<double const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<double volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<double const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<long double>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<long double const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<long double volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<long double const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<void*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<int*const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<f1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<f2>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<f3>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<mf1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<mf2>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<mf3>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<mp>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<cmf>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<enum_UDT>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<int&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<const int&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<int[2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<int[3][2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<int[2][4][5][6][3]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<empty_UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_new_operator<void>::value, false);

TT_TEST_END
