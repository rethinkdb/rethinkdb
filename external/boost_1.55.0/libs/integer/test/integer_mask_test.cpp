//  boost integer_mask.hpp test program  -------------------------------------//

//  (C) Copyright Daryle Walker 2001.
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

//  Revision History
//  23 Sep 2001  Initial version (Daryle Walker)

#include <boost/detail/lightweight_test.hpp>

#include <boost/cstdlib.hpp>               // for boost::exit_success
#include <boost/integer/integer_mask.hpp>  // for boost::high_bit_mask_t, etc.

#include <iostream>  // for std::cout (std::endl indirectly)

#ifdef BOOST_MSVC
#pragma warning(disable:4127) // conditional expression is constant
#endif

#if defined(BOOST_HAS_LONG_LONG)
#define MASK_TYPE ::boost::ulong_long_type
#elif defined(BOOST_HAS_MS_INT64)
#define MASK_TYPE unsigned __int64
#else
#define MASK_TYPE unsigned long
#endif

#define ONE (static_cast<MASK_TYPE>(1))

#define PRIVATE_HIGH_BIT_SLOW_TEST(v)  BOOST_TEST( ::boost::high_bit_mask_t< \
 (v) >::high_bit == (ONE << (v)) );
#define PRIVATE_HIGH_BIT_FAST_TEST(v)  BOOST_TEST( ::boost::high_bit_mask_t< \
 (v) >::high_bit_fast == (ONE << (v)) );
#define PRIVATE_HIGH_BIT_TEST(v)  do { PRIVATE_HIGH_BIT_SLOW_TEST(v); \
 PRIVATE_HIGH_BIT_FAST_TEST(v); } while (false)

#define PRIVATE_LOW_BITS_SLOW_TEST(v)  \
   do{ \
      MASK_TYPE mask = 0;\
      if(v > 0)\
         { mask = ((ONE << (v-1)) - 1); mask <<= 1; mask |= 1; }\
      BOOST_TEST( ::boost::low_bits_mask_t< (v) >::sig_bits ==  mask); \
   }while(false);
#define PRIVATE_LOW_BITS_FAST_TEST(v)  \
   do{ \
      MASK_TYPE mask = 0;\
      if(v > 0)\
         { mask = ((ONE << (v-1)) - 1); mask <<= 1; mask |= 1; }\
      BOOST_TEST( ::boost::low_bits_mask_t< (v) >::sig_bits_fast == mask);\
   }while(false);
#define PRIVATE_LOW_BITS_TEST(v)  do { PRIVATE_LOW_BITS_SLOW_TEST(v); \
 PRIVATE_LOW_BITS_FAST_TEST(v); } while (false)


int main( int, char*[] )
{
    using std::cout;
    using std::endl;

    cout << "Doing high_bit_mask_t tests." << endl;

#if defined(BOOST_HAS_LONG_LONG) || defined(BOOST_HAS_MS_INT64)
    PRIVATE_HIGH_BIT_TEST( 63 );
    PRIVATE_HIGH_BIT_TEST( 62 );
    PRIVATE_HIGH_BIT_TEST( 61 );
    PRIVATE_HIGH_BIT_TEST( 60 );
    PRIVATE_HIGH_BIT_TEST( 59 );
    PRIVATE_HIGH_BIT_TEST( 58 );
    PRIVATE_HIGH_BIT_TEST( 57 );
    PRIVATE_HIGH_BIT_TEST( 56 );
    PRIVATE_HIGH_BIT_TEST( 55 );
    PRIVATE_HIGH_BIT_TEST( 54 );
    PRIVATE_HIGH_BIT_TEST( 53 );
    PRIVATE_HIGH_BIT_TEST( 52 );
    PRIVATE_HIGH_BIT_TEST( 51 );
    PRIVATE_HIGH_BIT_TEST( 50 );
    PRIVATE_HIGH_BIT_TEST( 49 );
    PRIVATE_HIGH_BIT_TEST( 48 );
    PRIVATE_HIGH_BIT_TEST( 47 );
    PRIVATE_HIGH_BIT_TEST( 46 );
    PRIVATE_HIGH_BIT_TEST( 45 );
    PRIVATE_HIGH_BIT_TEST( 44 );
    PRIVATE_HIGH_BIT_TEST( 43 );
    PRIVATE_HIGH_BIT_TEST( 42 );
    PRIVATE_HIGH_BIT_TEST( 41 );
    PRIVATE_HIGH_BIT_TEST( 40 );
    PRIVATE_HIGH_BIT_TEST( 39 );
    PRIVATE_HIGH_BIT_TEST( 38 );
    PRIVATE_HIGH_BIT_TEST( 37 );
    PRIVATE_HIGH_BIT_TEST( 36 );
    PRIVATE_HIGH_BIT_TEST( 35 );
    PRIVATE_HIGH_BIT_TEST( 34 );
    PRIVATE_HIGH_BIT_TEST( 33 );
    PRIVATE_HIGH_BIT_TEST( 32 );
#endif
    PRIVATE_HIGH_BIT_TEST( 31 );
    PRIVATE_HIGH_BIT_TEST( 30 );
    PRIVATE_HIGH_BIT_TEST( 29 );
    PRIVATE_HIGH_BIT_TEST( 28 );
    PRIVATE_HIGH_BIT_TEST( 27 );
    PRIVATE_HIGH_BIT_TEST( 26 );
    PRIVATE_HIGH_BIT_TEST( 25 );
    PRIVATE_HIGH_BIT_TEST( 24 );
    PRIVATE_HIGH_BIT_TEST( 23 );
    PRIVATE_HIGH_BIT_TEST( 22 );
    PRIVATE_HIGH_BIT_TEST( 21 );
    PRIVATE_HIGH_BIT_TEST( 20 );
    PRIVATE_HIGH_BIT_TEST( 19 );
    PRIVATE_HIGH_BIT_TEST( 18 );
    PRIVATE_HIGH_BIT_TEST( 17 );
    PRIVATE_HIGH_BIT_TEST( 16 );
    PRIVATE_HIGH_BIT_TEST( 15 );
    PRIVATE_HIGH_BIT_TEST( 14 );
    PRIVATE_HIGH_BIT_TEST( 13 );
    PRIVATE_HIGH_BIT_TEST( 12 );
    PRIVATE_HIGH_BIT_TEST( 11 );
    PRIVATE_HIGH_BIT_TEST( 10 );
    PRIVATE_HIGH_BIT_TEST(  9 );
    PRIVATE_HIGH_BIT_TEST(  8 );
    PRIVATE_HIGH_BIT_TEST(  7 );
    PRIVATE_HIGH_BIT_TEST(  6 );
    PRIVATE_HIGH_BIT_TEST(  5 );
    PRIVATE_HIGH_BIT_TEST(  4 );
    PRIVATE_HIGH_BIT_TEST(  3 );
    PRIVATE_HIGH_BIT_TEST(  2 );
    PRIVATE_HIGH_BIT_TEST(  1 );
    PRIVATE_HIGH_BIT_TEST(  0 );

    cout << "Doing low_bits_mask_t tests." << endl;

#if defined(BOOST_HAS_LONG_LONG) || defined(BOOST_HAS_MS_INT64)
    PRIVATE_LOW_BITS_TEST( 64 );
    PRIVATE_LOW_BITS_TEST( 63 );
    PRIVATE_LOW_BITS_TEST( 62 );
    PRIVATE_LOW_BITS_TEST( 61 );
    PRIVATE_LOW_BITS_TEST( 60 );
    PRIVATE_LOW_BITS_TEST( 59 );
    PRIVATE_LOW_BITS_TEST( 58 );
    PRIVATE_LOW_BITS_TEST( 57 );
    PRIVATE_LOW_BITS_TEST( 56 );
    PRIVATE_LOW_BITS_TEST( 55 );
    PRIVATE_LOW_BITS_TEST( 54 );
    PRIVATE_LOW_BITS_TEST( 53 );
    PRIVATE_LOW_BITS_TEST( 52 );
    PRIVATE_LOW_BITS_TEST( 51 );
    PRIVATE_LOW_BITS_TEST( 50 );
    PRIVATE_LOW_BITS_TEST( 49 );
    PRIVATE_LOW_BITS_TEST( 48 );
    PRIVATE_LOW_BITS_TEST( 47 );
    PRIVATE_LOW_BITS_TEST( 46 );
    PRIVATE_LOW_BITS_TEST( 45 );
    PRIVATE_LOW_BITS_TEST( 44 );
    PRIVATE_LOW_BITS_TEST( 43 );
    PRIVATE_LOW_BITS_TEST( 42 );
    PRIVATE_LOW_BITS_TEST( 41 );
    PRIVATE_LOW_BITS_TEST( 40 );
    PRIVATE_LOW_BITS_TEST( 39 );
    PRIVATE_LOW_BITS_TEST( 38 );
    PRIVATE_LOW_BITS_TEST( 37 );
    PRIVATE_LOW_BITS_TEST( 36 );
    PRIVATE_LOW_BITS_TEST( 35 );
    PRIVATE_LOW_BITS_TEST( 34 );
    PRIVATE_LOW_BITS_TEST( 33 );
#endif
    PRIVATE_LOW_BITS_TEST( 32 );
    PRIVATE_LOW_BITS_TEST( 31 );
    PRIVATE_LOW_BITS_TEST( 30 );
    PRIVATE_LOW_BITS_TEST( 29 );
    PRIVATE_LOW_BITS_TEST( 28 );
    PRIVATE_LOW_BITS_TEST( 27 );
    PRIVATE_LOW_BITS_TEST( 26 );
    PRIVATE_LOW_BITS_TEST( 25 );
    PRIVATE_LOW_BITS_TEST( 24 );
    PRIVATE_LOW_BITS_TEST( 23 );
    PRIVATE_LOW_BITS_TEST( 22 );
    PRIVATE_LOW_BITS_TEST( 21 );
    PRIVATE_LOW_BITS_TEST( 20 );
    PRIVATE_LOW_BITS_TEST( 19 );
    PRIVATE_LOW_BITS_TEST( 18 );
    PRIVATE_LOW_BITS_TEST( 17 );
    PRIVATE_LOW_BITS_TEST( 16 );
    PRIVATE_LOW_BITS_TEST( 15 );
    PRIVATE_LOW_BITS_TEST( 14 );
    PRIVATE_LOW_BITS_TEST( 13 );
    PRIVATE_LOW_BITS_TEST( 12 );
    PRIVATE_LOW_BITS_TEST( 11 );
    PRIVATE_LOW_BITS_TEST( 10 );
    PRIVATE_LOW_BITS_TEST(  9 );
    PRIVATE_LOW_BITS_TEST(  8 );
    PRIVATE_LOW_BITS_TEST(  7 );
    PRIVATE_LOW_BITS_TEST(  6 );
    PRIVATE_LOW_BITS_TEST(  5 );
    PRIVATE_LOW_BITS_TEST(  4 );
    PRIVATE_LOW_BITS_TEST(  3 );
    PRIVATE_LOW_BITS_TEST(  2 );
    PRIVATE_LOW_BITS_TEST(  1 );

    return boost::report_errors();
}
