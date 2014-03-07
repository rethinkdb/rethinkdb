//  boost cstdint.hpp test program  ------------------------------------------//

//  Copyright Beman Dawes 2000.  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


//  See http://www.boost.org/libs/integer for documentation.

//  Revision History
//   11 Sep 01  Adapted to work with macros defined in native stdint.h (John Maddock)
//   12 Nov 00  Adapted to merged <boost/cstdint.hpp>
//   23 Sep 00  Added INTXX_C constant macro support + int64_t support (John Maddock).
//   28 Jun 00  Initial version

//
// There are two ways to test this: in version 1, we include cstdint.hpp as the first
// include, which means we get decide whether __STDC_CONSTANT_MACROS is defined.
// In version two we include stdint.h with __STDC_CONSTANT_MACROS *NOT* defined first,
// and check that we still end up with compatible definitions for the INT#_C macros.
//
// This is version 1.
//

#if defined(__GNUC__) && (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 4))
// We can't suppress this warning on the command line as not all GCC versions support -Wno-type-limits :
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

#include <boost/cstdint.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <iostream>

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//
// the following class is designed to verify
// that the various INTXX_C macros can be used
// in integral constant expressions:
//
struct integral_constant_checker
{
  static const boost::int8_t          int8          = INT8_C(-127);
  static const boost::int_least8_t    int_least8    = INT8_C(-127);
  static const boost::int_fast8_t     int_fast8     = INT8_C(-127);

  static const boost::uint8_t         uint8         = UINT8_C(255);
  static const boost::uint_least8_t   uint_least8   = UINT8_C(255);
  static const boost::uint_fast8_t    uint_fast8    = UINT8_C(255);

  static const boost::int16_t         int16         = INT16_C(-32767);
  static const boost::int_least16_t   int_least16   = INT16_C(-32767);
  static const boost::int_fast16_t    int_fast16    = INT16_C(-32767);

  static const boost::uint16_t        uint16         = UINT16_C(65535);
  static const boost::uint_least16_t  uint_least16   = UINT16_C(65535);
  static const boost::uint_fast16_t   uint_fast16    = UINT16_C(65535);

  static const boost::int32_t         int32         = INT32_C(-2147483647);
  static const boost::int_least32_t   int_least32   = INT32_C(-2147483647);
  static const boost::int_fast32_t    int_fast32    = INT32_C(-2147483647);

  static const boost::uint32_t        uint32        = UINT32_C(4294967295);
  static const boost::uint_least32_t  uint_least32  = UINT32_C(4294967295);
  static const boost::uint_fast32_t   uint_fast32   = UINT32_C(4294967295);

  static void check();
};

void integral_constant_checker::check()
{
  BOOST_TEST( int8 == -127 );
  BOOST_TEST( int_least8 == -127 );
  BOOST_TEST( int_fast8 == -127 );
  BOOST_TEST( uint8 == 255u );
  BOOST_TEST( uint_least8 == 255u );
  BOOST_TEST( uint_fast8 == 255u );
  BOOST_TEST( int16 == -32767 );
  BOOST_TEST( int_least16 == -32767 );
  BOOST_TEST( int_fast16 == -32767 );
  BOOST_TEST( uint16 == 65535u );
  BOOST_TEST( uint_least16 == 65535u );
  BOOST_TEST( uint_fast16 == 65535u );
  BOOST_TEST( int32 == -2147483647 );
  BOOST_TEST( int_least32 == -2147483647 );
  BOOST_TEST( int_fast32 == -2147483647 );
  BOOST_TEST( uint32 == 4294967295u );
  BOOST_TEST( uint_least32 == 4294967295u );
  BOOST_TEST( uint_fast32 == 4294967295u );
}
#endif // BOOST_NO_INCLASS_MEMBER_INITIALIZATION

//
// the following function simply verifies that the type
// of an integral constant is correctly defined:
//
#ifdef __BORLANDC__
#pragma option -w-8008
#pragma option -w-8066
#endif
template <class T1, class T2>
void integral_constant_type_check(T1, T2)
{
   //
   // the types T1 and T2 may not be exactly
   // the same type, but they should be the
   // same size and signedness. We could use
   // numeric_limits to verify this, but
   // numeric_limits implementations currently
   // vary too much, or are incomplete or missing.
   //
   T1 t1 = static_cast<T1>(-1);  // cast suppresses warnings
   T2 t2 = static_cast<T2>(-1);  // ditto
#if defined(BOOST_HAS_STDINT_H)
   // if we have a native stdint.h
   // then the INTXX_C macros may define
   // a type that's wider than required:
   BOOST_TEST(sizeof(T1) <= sizeof(T2));
#else
   BOOST_TEST(sizeof(T1) == sizeof(T2));
   BOOST_TEST(t1 == t2);
#endif
#if defined(BOOST_HAS_STDINT_H)
   // native headers are permitted to promote small
   // unsigned types to type int:
   if(sizeof(T1) >= sizeof(int))
   {
      if(t1 > 0)
        BOOST_TEST(t2 > 0);
      else
        BOOST_TEST(!(t2 > 0));
   }
   else if(t1 < 0)
      BOOST_TEST(!(t2 > 0));
#else
   if(t1 > 0)
     BOOST_TEST(t2 > 0);
   else
     BOOST_TEST(!(t2 > 0));
#endif
}


int main(int, char*[])
{
#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
  integral_constant_checker::check();
#endif
  //
  // verify the types of the integral constants:
  //
  integral_constant_type_check(boost::int8_t(0), INT8_C(0));
  integral_constant_type_check(boost::uint8_t(0), UINT8_C(0));
  integral_constant_type_check(boost::int16_t(0), INT16_C(0));
  integral_constant_type_check(boost::uint16_t(0), UINT16_C(0));
  integral_constant_type_check(boost::int32_t(0), INT32_C(0));
  integral_constant_type_check(boost::uint32_t(0), UINT32_C(0));
#ifndef BOOST_NO_INT64_T
  integral_constant_type_check(boost::int64_t(0), INT64_C(0));
  integral_constant_type_check(boost::uint64_t(0), UINT64_C(0));
#endif
  //
  boost::int8_t          int8          = INT8_C(-127);
  boost::int_least8_t    int_least8    = INT8_C(-127);
  boost::int_fast8_t     int_fast8     = INT8_C(-127);

  boost::uint8_t         uint8         = UINT8_C(255);
  boost::uint_least8_t   uint_least8   = UINT8_C(255);
  boost::uint_fast8_t    uint_fast8    = UINT8_C(255);

  boost::int16_t         int16         = INT16_C(-32767);
  boost::int_least16_t   int_least16   = INT16_C(-32767);
  boost::int_fast16_t    int_fast16    = INT16_C(-32767);

  boost::uint16_t        uint16         = UINT16_C(65535);
  boost::uint_least16_t  uint_least16   = UINT16_C(65535);
  boost::uint_fast16_t   uint_fast16    = UINT16_C(65535);

  boost::int32_t         int32         = INT32_C(-2147483647);
  boost::int_least32_t   int_least32   = INT32_C(-2147483647);
  boost::int_fast32_t    int_fast32    = INT32_C(-2147483647);

  boost::uint32_t        uint32        = UINT32_C(4294967295);
  boost::uint_least32_t  uint_least32  = UINT32_C(4294967295);
  boost::uint_fast32_t   uint_fast32   = UINT32_C(4294967295);

#ifndef BOOST_NO_INT64_T
  boost::int64_t         int64         = INT64_C(-9223372036854775807);
  boost::int_least64_t   int_least64   = INT64_C(-9223372036854775807);
  boost::int_fast64_t    int_fast64    = INT64_C(-9223372036854775807);

  boost::uint64_t        uint64        = UINT64_C(18446744073709551615);
  boost::uint_least64_t  uint_least64  = UINT64_C(18446744073709551615);
  boost::uint_fast64_t   uint_fast64   = UINT64_C(18446744073709551615);

  boost::intmax_t        intmax        = INTMAX_C(-9223372036854775807);
  boost::uintmax_t       uintmax       = UINTMAX_C(18446744073709551615);
#else
  boost::intmax_t        intmax        = INTMAX_C(-2147483647);
  boost::uintmax_t       uintmax       = UINTMAX_C(4294967295);
#endif

  BOOST_TEST( int8 == -127 );
  BOOST_TEST( int_least8 == -127 );
  BOOST_TEST( int_fast8 == -127 );
  BOOST_TEST( uint8 == 255u );
  BOOST_TEST( uint_least8 == 255u );
  BOOST_TEST( uint_fast8 == 255u );
  BOOST_TEST( int16 == -32767 );
  BOOST_TEST( int_least16 == -32767 );
  BOOST_TEST( int_fast16 == -32767 );
  BOOST_TEST( uint16 == 65535u );
  BOOST_TEST( uint_least16 == 65535u );
  BOOST_TEST( uint_fast16 == 65535u );
  BOOST_TEST( int32 == -2147483647 );
  BOOST_TEST( int_least32 == -2147483647 );
  BOOST_TEST( int_fast32 == -2147483647 );
  BOOST_TEST( uint32 == 4294967295u );
  BOOST_TEST( uint_least32 == 4294967295u );
  BOOST_TEST( uint_fast32 == 4294967295u );

#ifndef BOOST_NO_INT64_T
  BOOST_TEST( int64 == INT64_C(-9223372036854775807) );
  BOOST_TEST( int_least64 == INT64_C(-9223372036854775807) );
  BOOST_TEST( int_fast64 == INT64_C(-9223372036854775807) );
  BOOST_TEST( uint64 == UINT64_C(18446744073709551615) );
  BOOST_TEST( uint_least64 == UINT64_C(18446744073709551615) );
  BOOST_TEST( uint_fast64 == UINT64_C(18446744073709551615) );
  BOOST_TEST( intmax == INT64_C(-9223372036854775807) );
  BOOST_TEST( uintmax == UINT64_C(18446744073709551615) );
#else
  BOOST_TEST( intmax == -2147483647 );
  BOOST_TEST( uintmax == 4294967295u );
#endif


  std::cout << "OK\n";
  return boost::report_errors();
}
