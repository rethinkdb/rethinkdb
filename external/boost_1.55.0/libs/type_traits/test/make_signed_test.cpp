
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
#  include <boost/type_traits/make_signed.hpp>
#endif

TT_TEST_BEGIN(make_signed)
// signed types:
BOOST_CHECK_TYPE(::tt::make_signed<signed char>::type, signed char);
BOOST_CHECK_TYPE(::tt::make_signed<short>::type, short);
BOOST_CHECK_TYPE(::tt::make_signed<int>::type, int);
BOOST_CHECK_TYPE(::tt::make_signed<long>::type, long);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_TYPE(::tt::make_signed<boost::long_long_type>::type, boost::long_long_type);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_TYPE(::tt::make_signed<__int64>::type, __int64);
#endif
// const signed types:
BOOST_CHECK_TYPE(::tt::make_signed<const signed char>::type, const signed char);
BOOST_CHECK_TYPE(::tt::make_signed<const short>::type, const short);
BOOST_CHECK_TYPE(::tt::make_signed<const int>::type, const int);
BOOST_CHECK_TYPE(::tt::make_signed<const long>::type, const long);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_TYPE(::tt::make_signed<const boost::long_long_type>::type, const boost::long_long_type);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_TYPE(::tt::make_signed<const __int64>::type, const __int64);
#endif
// volatile signed types:
BOOST_CHECK_TYPE(::tt::make_signed<volatile signed char>::type, volatile signed char);
BOOST_CHECK_TYPE(::tt::make_signed<volatile short>::type, volatile short);
BOOST_CHECK_TYPE(::tt::make_signed<volatile int>::type, volatile int);
BOOST_CHECK_TYPE(::tt::make_signed<volatile long>::type, volatile long);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_TYPE(::tt::make_signed<volatile boost::long_long_type>::type, volatile boost::long_long_type);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_TYPE(::tt::make_signed<volatile __int64>::type, volatile __int64);
#endif
// const volatile signed types:
BOOST_CHECK_TYPE(::tt::make_signed<const volatile signed char>::type, const volatile signed char);
BOOST_CHECK_TYPE(::tt::make_signed<const volatile short>::type, const volatile short);
BOOST_CHECK_TYPE(::tt::make_signed<const volatile int>::type, const volatile int);
BOOST_CHECK_TYPE(::tt::make_signed<const volatile long>::type, const volatile long);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_TYPE(::tt::make_signed<const volatile boost::long_long_type>::type, const volatile boost::long_long_type);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_TYPE(::tt::make_signed<const volatile __int64>::type, const volatile __int64);
#endif

// unsigned types:
BOOST_CHECK_TYPE(::tt::make_signed<unsigned char>::type, signed char);
BOOST_CHECK_TYPE(::tt::make_signed<unsigned short>::type, short);
BOOST_CHECK_TYPE(::tt::make_signed<unsigned int>::type, int);
BOOST_CHECK_TYPE(::tt::make_signed<unsigned long>::type, long);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_TYPE(::tt::make_signed<boost::ulong_long_type>::type, boost::long_long_type);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_TYPE(::tt::make_signed<unsigned __int64>::type, __int64);
#endif
// const unsigned types:
BOOST_CHECK_TYPE(::tt::make_signed<const unsigned char>::type, const signed char);
BOOST_CHECK_TYPE(::tt::make_signed<const unsigned short>::type, const short);
BOOST_CHECK_TYPE(::tt::make_signed<const unsigned int>::type, const int);
BOOST_CHECK_TYPE(::tt::make_signed<const unsigned long>::type, const long);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_TYPE(::tt::make_signed<const boost::ulong_long_type>::type, const boost::long_long_type);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_TYPE(::tt::make_signed<const unsigned __int64>::type, const __int64);
#endif
// volatile unsigned types:
BOOST_CHECK_TYPE(::tt::make_signed<volatile unsigned char>::type, volatile signed char);
BOOST_CHECK_TYPE(::tt::make_signed<volatile unsigned short>::type, volatile short);
BOOST_CHECK_TYPE(::tt::make_signed<volatile unsigned int>::type, volatile int);
BOOST_CHECK_TYPE(::tt::make_signed<volatile unsigned long>::type, volatile long);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_TYPE(::tt::make_signed<volatile boost::ulong_long_type>::type, volatile boost::long_long_type);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_TYPE(::tt::make_signed<volatile unsigned __int64>::type, volatile __int64);
#endif
// const volatile unsigned types:
BOOST_CHECK_TYPE(::tt::make_signed<const volatile unsigned char>::type, const volatile signed char);
BOOST_CHECK_TYPE(::tt::make_signed<const volatile unsigned short>::type, const volatile short);
BOOST_CHECK_TYPE(::tt::make_signed<const volatile unsigned int>::type, const volatile int);
BOOST_CHECK_TYPE(::tt::make_signed<const volatile unsigned long>::type, const volatile long);
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_TYPE(::tt::make_signed<const volatile boost::ulong_long_type>::type, const volatile boost::long_long_type);
#elif defined(BOOST_HAS_MS_INT64)
BOOST_CHECK_TYPE(::tt::make_signed<const volatile unsigned __int64>::type, const volatile __int64);
#endif
#ifdef BOOST_HAS_INT128
BOOST_CHECK_TYPE(::tt::make_signed<boost::int128_type>::type, boost::int128_type);
BOOST_CHECK_TYPE(::tt::make_signed<boost::uint128_type>::type, boost::int128_type);
#endif

// character types:
BOOST_CHECK_TYPE(::tt::make_signed<char>::type, signed char);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral< ::tt::make_signed<wchar_t>::type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed< ::tt::make_signed<wchar_t>::type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral< ::tt::make_signed<enum_UDT>::type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed< ::tt::make_signed<enum_UDT>::type>::value, true);
TT_TEST_END


