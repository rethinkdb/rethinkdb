
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/alignment_of.hpp>
#  include <boost/type_traits/type_with_alignment.hpp>
#  include <boost/type_traits/is_pod.hpp>
#endif

#if defined(BOOST_MSVC) || (defined(BOOST_INTEL) && defined(_MSC_VER))
#if (_MSC_VER >= 1400) && defined(_M_IX86)
#include <xmmintrin.h>
#endif
struct __declspec(align(8)) a8 { char m[8]; };
struct __declspec(align(16)) a16 { char m[16]; };
struct __declspec(align(32)) a32 { char m[32]; };
struct __declspec(align(64)) a64 { char m[64]; };
struct __declspec(align(128)) a128 { char m[128]; };
#endif

void check_call2(...){}

template <class T>
void check_call(const T& v)
{
   check_call2(v);
}

#define TYPE_WITH_ALIGNMENT_TEST(T)\
{\
std::cout << "\ntesting type " << typeid(T).name() << std::endl;\
std::cout << "Alignment of T is " << ::tt::alignment_of< T >::value << std::endl;\
std::cout << "Aligned type is " << typeid(::tt::type_with_alignment< ::tt::alignment_of< T >::value>::type).name() << std::endl;\
std::cout << "Alignment of aligned type is " << ::tt::alignment_of<\
   ::tt::type_with_alignment<\
      ::tt::alignment_of< T >::value\
   >::type\
>::value << std::endl;\
BOOST_CHECK(::tt::alignment_of<\
               ::tt::type_with_alignment<\
                  ::tt::alignment_of< T >::value\
               >::type\
            >::value == ::boost::alignment_of< T >::value);\
BOOST_CHECK(::tt::is_pod<\
               ::tt::type_with_alignment<\
                  ::tt::alignment_of< T >::value>::type\
            >::value);\
}
#define TYPE_WITH_ALIGNMENT_TEST_EX(T)\
   TYPE_WITH_ALIGNMENT_TEST(T)\
{\
   ::tt::type_with_alignment<\
      ::tt::alignment_of< T >::value\
   >::type val;\
   check_call(val);\
}


TT_TEST_BEGIN(type_with_alignment)

TYPE_WITH_ALIGNMENT_TEST_EX(char)
TYPE_WITH_ALIGNMENT_TEST_EX(short)
TYPE_WITH_ALIGNMENT_TEST_EX(int)
TYPE_WITH_ALIGNMENT_TEST_EX(long)
TYPE_WITH_ALIGNMENT_TEST_EX(float)
TYPE_WITH_ALIGNMENT_TEST_EX(double)
TYPE_WITH_ALIGNMENT_TEST_EX(long double)

#ifdef BOOST_HAS_LONG_LONG
TYPE_WITH_ALIGNMENT_TEST_EX(::boost::long_long_type)
#endif
#ifdef BOOST_HAS_MS_INT64
TYPE_WITH_ALIGNMENT_TEST_EX(__int64)
#endif
TYPE_WITH_ALIGNMENT_TEST_EX(int[4])
TYPE_WITH_ALIGNMENT_TEST_EX(int(*)(int))
TYPE_WITH_ALIGNMENT_TEST_EX(int*)
TYPE_WITH_ALIGNMENT_TEST_EX(VB)
TYPE_WITH_ALIGNMENT_TEST_EX(VD)
TYPE_WITH_ALIGNMENT_TEST_EX(enum_UDT)
TYPE_WITH_ALIGNMENT_TEST_EX(mf2)
TYPE_WITH_ALIGNMENT_TEST_EX(POD_UDT)
TYPE_WITH_ALIGNMENT_TEST_EX(empty_UDT)
TYPE_WITH_ALIGNMENT_TEST_EX(union_UDT)

#if defined(BOOST_MSVC) || (defined(BOOST_INTEL) && defined(_MSC_VER))
#if (_MSC_VER >= 1400) && defined(_M_IX86)
TYPE_WITH_ALIGNMENT_TEST(__m128)
TYPE_WITH_ALIGNMENT_TEST(__m64)
#endif
TYPE_WITH_ALIGNMENT_TEST(a8)
TYPE_WITH_ALIGNMENT_TEST(a16)
TYPE_WITH_ALIGNMENT_TEST(a32)
#endif

TT_TEST_END









