//  Copyright John Maddock 2009.  
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define __STDC_CONSTANT_MACROS
#include <boost/cstdint.hpp> // must be the only #include!

int main()
{
   boost::int8_t i8 = INT8_C(0);
   (void)i8;
   boost::uint8_t ui8 = UINT8_C(0);
   (void)ui8;
   boost::int16_t i16 = INT16_C(0);
   (void)i16;
   boost::uint16_t ui16 = UINT16_C(0);
   (void)ui16;
   boost::int32_t i32 = INT32_C(0);
   (void)i32;
   boost::uint32_t ui32 = UINT32_C(0);
   (void)ui32;
#ifndef BOOST_NO_INT64_T
   boost::int64_t i64 = 0;
   (void)i64;
   boost::uint64_t ui64 = 0;
   (void)ui64;
#endif
   boost::int_least8_t i8least = INT8_C(0);
   (void)i8least;
   boost::uint_least8_t ui8least = UINT8_C(0);
   (void)ui8least;
   boost::int_least16_t i16least = INT16_C(0);
   (void)i16least;
   boost::uint_least16_t ui16least = UINT16_C(0);
   (void)ui16least;
   boost::int_least32_t i32least = INT32_C(0);
   (void)i32least;
   boost::uint_least32_t ui32least = UINT32_C(0);
   (void)ui32least;
#ifndef BOOST_NO_INT64_T
   boost::int_least64_t i64least = 0;
   (void)i64least;
   boost::uint_least64_t ui64least = 0;
   (void)ui64least;
#endif
   boost::int_fast8_t i8fast = INT8_C(0);
   (void)i8fast;
   boost::uint_fast8_t ui8fast = UINT8_C(0);
   (void)ui8fast;
   boost::int_fast16_t i16fast = INT16_C(0);
   (void)i16fast;
   boost::uint_fast16_t ui16fast = UINT16_C(0);
   (void)ui16fast;
   boost::int_fast32_t i32fast = INT32_C(0);
   (void)i32fast;
   boost::uint_fast32_t ui32fast = UINT32_C(0);
   (void)ui32fast;
#ifndef BOOST_NO_INT64_T
   boost::int_fast64_t i64fast = 0;
   (void)i64fast;
   boost::uint_fast64_t ui64fast = 0;
   (void)ui64fast;
#endif
   boost::intmax_t im = 0;
   (void)im;
   boost::uintmax_t uim = 0;
   (void)uim;
}
