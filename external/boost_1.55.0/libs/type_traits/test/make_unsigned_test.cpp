
//  (C) Copyright John Maddock 2007. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.tt.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_type.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/make_unsigned.hpp>
#endif

TT_TEST_BEGIN(make_unsigned)
// signed types:
BOOST_CHECK_TYPE(::tt::make_unsigned<signed char>::type, unsigned char);
BOOST_CHECK_TYPE(::tt::make_unsigned<short>::type, unsigned short);
BOOST_CHECK_TYPE(::tt::make_unsigned<int>::type, unsigned int);
BOOST_CHECK_TYPE(::tt::make_unsigned<long>::type, unsigned long);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_TYPE(::tt::make_unsigned<boost::long_long_type>::type, boost::ulong_long_type);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_TYPE(::tt::make_unsigned<__int64>::type, unsigned __int64);
#endif
// const signed types:
BOOST_CHECK_TYPE(::tt::make_unsigned<const signed char>::type, const unsigned char);
BOOST_CHECK_TYPE(::tt::make_unsigned<const short>::type, const unsigned short);
BOOST_CHECK_TYPE(::tt::make_unsigned<const int>::type, const unsigned int);
BOOST_CHECK_TYPE(::tt::make_unsigned<const long>::type, const unsigned long);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_TYPE(::tt::make_unsigned<const boost::long_long_type>::type, const boost::ulong_long_type);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_TYPE(::tt::make_unsigned<const __int64>::type, const unsigned __int64);
#endif
// volatile signed types:
BOOST_CHECK_TYPE(::tt::make_unsigned<volatile signed char>::type, volatile unsigned char);
BOOST_CHECK_TYPE(::tt::make_unsigned<volatile short>::type, volatile unsigned short);
BOOST_CHECK_TYPE(::tt::make_unsigned<volatile int>::type, volatile unsigned int);
BOOST_CHECK_TYPE(::tt::make_unsigned<volatile long>::type, volatile unsigned long);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_TYPE(::tt::make_unsigned<volatile boost::long_long_type>::type, volatile boost::ulong_long_type);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_TYPE(::tt::make_unsigned<volatile __int64>::type, volatile unsigned __int64);
#endif
// const volatile signed types:
BOOST_CHECK_TYPE(::tt::make_unsigned<const volatile signed char>::type, const volatile unsigned char);
BOOST_CHECK_TYPE(::tt::make_unsigned<const volatile short>::type, const volatile unsigned short);
BOOST_CHECK_TYPE(::tt::make_unsigned<const volatile int>::type, const volatile unsigned int);
BOOST_CHECK_TYPE(::tt::make_unsigned<const volatile long>::type, const volatile unsigned long);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_TYPE(::tt::make_unsigned<const volatile boost::long_long_type>::type, const volatile boost::ulong_long_type);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_TYPE(::tt::make_unsigned<const volatile __int64>::type, const volatile unsigned __int64);
#endif

// unsigned types:
BOOST_CHECK_TYPE(::tt::make_unsigned<unsigned char>::type, unsigned char);
BOOST_CHECK_TYPE(::tt::make_unsigned<unsigned short>::type, unsigned short);
BOOST_CHECK_TYPE(::tt::make_unsigned<unsigned int>::type, unsigned int);
BOOST_CHECK_TYPE(::tt::make_unsigned<unsigned long>::type, unsigned long);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_TYPE(::tt::make_unsigned<boost::ulong_long_type>::type, boost::ulong_long_type);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_TYPE(::tt::make_unsigned<unsigned __int64>::type, unsigned __int64);
#endif
// const unsigned types:
BOOST_CHECK_TYPE(::tt::make_unsigned<const unsigned char>::type, const unsigned char);
BOOST_CHECK_TYPE(::tt::make_unsigned<const unsigned short>::type, const unsigned short);
BOOST_CHECK_TYPE(::tt::make_unsigned<const unsigned int>::type, const unsigned int);
BOOST_CHECK_TYPE(::tt::make_unsigned<const unsigned long>::type, const unsigned long);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_TYPE(::tt::make_unsigned<const boost::ulong_long_type>::type, const boost::ulong_long_type);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_TYPE(::tt::make_unsigned<const unsigned __int64>::type, const unsigned __int64);
#endif
// volatile unsigned types:
BOOST_CHECK_TYPE(::tt::make_unsigned<volatile unsigned char>::type, volatile unsigned char);
BOOST_CHECK_TYPE(::tt::make_unsigned<volatile unsigned short>::type, volatile unsigned short);
BOOST_CHECK_TYPE(::tt::make_unsigned<volatile unsigned int>::type, volatile unsigned int);
BOOST_CHECK_TYPE(::tt::make_unsigned<volatile unsigned long>::type, volatile unsigned long);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_TYPE(::tt::make_unsigned<volatile boost::ulong_long_type>::type, volatile boost::ulong_long_type);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_TYPE(::tt::make_unsigned<volatile unsigned __int64>::type, volatile unsigned __int64);
#endif
// const volatile unsigned types:
BOOST_CHECK_TYPE(::tt::make_unsigned<const volatile unsigned char>::type, const volatile unsigned char);
BOOST_CHECK_TYPE(::tt::make_unsigned<const volatile unsigned short>::type, const volatile unsigned short);
BOOST_CHECK_TYPE(::tt::make_unsigned<const volatile unsigned int>::type, const volatile unsigned int);
BOOST_CHECK_TYPE(::tt::make_unsigned<const volatile unsigned long>::type, const volatile unsigned long);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_TYPE(::tt::make_unsigned<const volatile boost::ulong_long_type>::type, const volatile boost::ulong_long_type);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_TYPE(::tt::make_unsigned<const volatile unsigned __int64>::type, const volatile unsigned __int64);
#endif
#ifdef BOOST_HAS_INT128
BOOST_CHECK_TYPE(::tt::make_unsigned<boost::int128_type>::type, boost::uint128_type);
BOOST_CHECK_TYPE(::tt::make_unsigned<boost::uint128_type>::type, boost::uint128_type);
#endif

// character types:
BOOST_CHECK_TYPE(::tt::make_unsigned<char>::type, unsigned char);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral< ::tt::make_unsigned<wchar_t>::type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_unsigned< ::tt::make_unsigned<wchar_t>::type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral< ::tt::make_unsigned<enum_UDT>::type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_unsigned< ::tt::make_unsigned<enum_UDT>::type>::value, true);
TT_TEST_END


