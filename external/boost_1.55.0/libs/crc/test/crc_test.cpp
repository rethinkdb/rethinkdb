//  Boost CRC test program file  ---------------------------------------------//

//  Copyright 2001, 2003, 2004 Daryle Walker.  Use, modification, and
//  distribution are subject to the Boost Software License, Version 1.0.  (See
//  accompanying file LICENSE_1_0.txt or a copy at
//  <http://www.boost.org/LICENSE_1_0.txt>.)

//  See <http://www.boost.org/libs/crc/> for the library's home page.

//  Revision History
//  28 Aug 2004  Added CRC tests for polynominals shorter than 8 bits
//               (Daryle Walker, by patch from Bert Klaps)
//  23 Aug 2003  Adjust to updated Test framework (Daryle Walker)
//  14 May 2001  Initial version (Daryle Walker)


#include <boost/config.hpp>                      // for BOOST_MSVC, etc.
#include <boost/crc.hpp>                         // for boost::crc_basic, etc.
#include <boost/cstdint.hpp>                     // for boost::uint16_t, etc.
#include <boost/cstdlib.hpp>                     // for boost::exit_success
#include <boost/integer.hpp>                     // for boost::uint_t
#include <boost/random/linear_congruential.hpp>  // for boost::minstd_rand
#include <boost/test/minimal.hpp>                // for main, etc.
#include <boost/timer.hpp>                       // for boost::timer

#include <algorithm>  // for std::for_each, std::generate_n, std::count
#include <climits>    // for CHAR_BIT
#include <cstddef>    // for std::size_t
#include <iostream>   // for std::cout (std::ostream and std::endl indirectly)


#if CHAR_BIT != 8
#error The expected results assume an eight-bit byte.
#endif

#if !(defined(BOOST_NO_DEPENDENT_TYPES_IN_TEMPLATE_VALUE_PARAMETERS) || (defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)))
#define CRC_PARM_TYPE  typename boost::uint_t<Bits>::fast
#else
#define CRC_PARM_TYPE  unsigned long
#endif

#if !defined(BOOST_MSVC) && !defined(__GNUC__)
#define PRIVATE_DECLARE_BOOST( TypeName )  using boost:: TypeName
#else
#define PRIVATE_DECLARE_BOOST( TypeName )  typedef boost:: TypeName  TypeName
#endif


// Types
template < std::size_t Bits, CRC_PARM_TYPE TrPo, CRC_PARM_TYPE InRe,
           CRC_PARM_TYPE FiXo, bool ReIn, bool ReRe >
class crc_tester
{
public:
    // All the following were separate function templates, but they have
    // been moved to class-static member functions of a class template
    // because MS VC++ 6 can't handle function templates that can't
    // deduce all their template arguments from their function arguments.

    typedef typename boost::uint_t<Bits>::fast  value_type;

    static  void  master_test( char const *test_name, value_type expected );

private:
    typedef boost::crc_optimal<Bits, TrPo, InRe, FiXo, ReIn, ReRe>
      optimal_crc_type;
    typedef boost::crc_basic<Bits>  basic_crc_type;

    static  void  compute_test( value_type expected );
    static  void  interrupt_test( value_type expected );
    static  void  error_test();

};  // crc_tester

// Global data
unsigned char const  std_data[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                      0x38, 0x39 };
std::size_t const    std_data_len = sizeof( std_data ) / sizeof( std_data[0] );

boost::uint16_t const  std_crc_ccitt_result = 0x29B1;
boost::uint16_t const  std_crc_16_result = 0xBB3D;
boost::uint32_t const  std_crc_32_result = 0xCBF43926;

// Function prototypes
void             timing_test();
boost::uint32_t  basic_crc32( void const *buffer, std::size_t byte_count );
boost::uint32_t  optimal_crc32( void const *buffer, std::size_t byte_count );
boost::uint32_t  quick_crc32( void const *buffer, std::size_t byte_count );
boost::uint32_t  quick_reflect( boost::uint32_t value, std::size_t bits );
double           time_trial( char const *name,
 boost::uint32_t (*crc_func)(void const *, std::size_t),
 boost::uint32_t expected, void const *data, std::size_t length );

void             augmented_tests();
boost::uint32_t  native_to_big( boost::uint32_t x );
boost::uint32_t  big_to_native( boost::uint32_t x );

void  small_crc_test1();
void  small_crc_test2();


// Macro to compact code
#define PRIVATE_TESTER_NAME  crc_tester<Bits, TrPo, InRe, FiXo, ReIn, ReRe>

// Run a test on slow and fast CRC computers and function
template < std::size_t Bits, CRC_PARM_TYPE TrPo, CRC_PARM_TYPE InRe,
           CRC_PARM_TYPE FiXo, bool ReIn, bool ReRe >
void
PRIVATE_TESTER_NAME::compute_test
(
    typename PRIVATE_TESTER_NAME::value_type  expected
)
{
    std::cout << "\tDoing computation tests." << std::endl;

    optimal_crc_type  fast_crc;
    basic_crc_type    slow_crc( TrPo, InRe, FiXo, ReIn, ReRe );
    value_type const  func_result = boost::crc<Bits, TrPo, InRe, FiXo, ReIn,
     ReRe>( std_data, std_data_len );

    fast_crc.process_bytes( std_data, std_data_len );
    slow_crc.process_bytes( std_data, std_data_len );
    BOOST_CHECK( fast_crc.checksum() == expected );
    BOOST_CHECK( slow_crc.checksum() == expected );
    BOOST_CHECK( func_result == expected );
}

// Run a test in two runs, and check all the inspectors
template < std::size_t Bits, CRC_PARM_TYPE TrPo, CRC_PARM_TYPE InRe,
           CRC_PARM_TYPE FiXo, bool ReIn, bool ReRe >
void
PRIVATE_TESTER_NAME::interrupt_test
(
    typename PRIVATE_TESTER_NAME::value_type  expected
)
{
    std::cout << "\tDoing interrupt tests." << std::endl;

    // Process the first half of the data (also test accessors)
    optimal_crc_type  fast_crc1;
    basic_crc_type    slow_crc1( fast_crc1.get_truncated_polynominal(),
     fast_crc1.get_initial_remainder(), fast_crc1.get_final_xor_value(),
     fast_crc1.get_reflect_input(), fast_crc1.get_reflect_remainder() );

    BOOST_CHECK( fast_crc1.get_interim_remainder() ==
     slow_crc1.get_initial_remainder() );

    std::size_t const            mid_way = std_data_len / 2;
    unsigned char const * const  std_data_end = std_data + std_data_len;

    fast_crc1.process_bytes( std_data, mid_way );
    slow_crc1.process_bytes( std_data, mid_way );
    BOOST_CHECK( fast_crc1.checksum() == slow_crc1.checksum() );

    // Process the second half of the data (also test accessors)
    boost::crc_optimal<optimal_crc_type::bit_count,
     optimal_crc_type::truncated_polynominal, optimal_crc_type::initial_remainder,
     optimal_crc_type::final_xor_value, optimal_crc_type::reflect_input,
     optimal_crc_type::reflect_remainder>
      fast_crc2( fast_crc1.get_interim_remainder() );
    boost::crc_basic<basic_crc_type::bit_count>  slow_crc2(
     slow_crc1.get_truncated_polynominal(), slow_crc1.get_interim_remainder(),
     slow_crc1.get_final_xor_value(), slow_crc1.get_reflect_input(),
     slow_crc1.get_reflect_remainder() );

    fast_crc2.process_block( std_data + mid_way, std_data_end );
    slow_crc2.process_block( std_data + mid_way, std_data_end );
    BOOST_CHECK( fast_crc2.checksum() == slow_crc2.checksum() );
    BOOST_CHECK( fast_crc2.checksum() == expected );
    BOOST_CHECK( slow_crc2.checksum() == expected );
}

// Run a test to see if a single-bit error is detected
template < std::size_t Bits, CRC_PARM_TYPE TrPo, CRC_PARM_TYPE InRe,
           CRC_PARM_TYPE FiXo, bool ReIn, bool ReRe >
void
PRIVATE_TESTER_NAME::error_test
(
)
{
    PRIVATE_DECLARE_BOOST( uint32_t );

    // A single-bit error is ensured to be detected if the polynominal
    // has at least two bits set.  The highest bit is what is removed
    // to give the truncated polynominal, and it is always set.  This
    // means that the truncated polynominal needs at least one of its
    // bits set, which implies that it cannot be zero.
    if ( !(TrPo & boost::detail::mask_uint_t<Bits>::sig_bits_fast) )
    {
        BOOST_FAIL( "truncated CRC polymonial is zero" );
    }

    std::cout << "\tDoing error tests." << std::endl;

    // Create a random block of data
    uint32_t           ran_data[ 256 ];
    std::size_t const  ran_length = sizeof(ran_data) / sizeof(ran_data[0]);

    std::generate_n( ran_data, ran_length, boost::minstd_rand() );

    // Create computers and compute the checksum of the data
    optimal_crc_type  fast_tester;
    basic_crc_type    slow_tester( TrPo, InRe, FiXo, ReIn, ReRe );

    fast_tester.process_bytes( ran_data, sizeof(ran_data) );
    slow_tester.process_bytes( ran_data, sizeof(ran_data) );

    uint32_t const  fast_checksum = fast_tester.checksum();
    uint32_t const  slow_checksum = slow_tester.checksum();

    BOOST_CHECK( fast_checksum == slow_checksum );

    // Do the checksum again (and test resetting ability)
    fast_tester.reset();
    slow_tester.reset( InRe );
    fast_tester.process_bytes( ran_data, sizeof(ran_data) );
    slow_tester.process_bytes( ran_data, sizeof(ran_data) );
    BOOST_CHECK( fast_tester.checksum() == slow_tester.checksum() );
    BOOST_CHECK( fast_tester.checksum() == fast_checksum );
    BOOST_CHECK( slow_tester.checksum() == slow_checksum );

    // Produce a single-bit error
    ran_data[ ran_data[0] % ran_length ] ^= ( 1 << (ran_data[1] % 32) );

    // Compute the checksum of the errorenous data
    // (and continue testing resetting ability)
    fast_tester.reset( InRe );
    slow_tester.reset();
    fast_tester.process_bytes( ran_data, sizeof(ran_data) );
    slow_tester.process_bytes( ran_data, sizeof(ran_data) );
    BOOST_CHECK( fast_tester.checksum() == slow_tester.checksum() );
    BOOST_CHECK( fast_tester.checksum() != fast_checksum );
    BOOST_CHECK( slow_tester.checksum() != slow_checksum );
}

// Run the other CRC object tests
template < std::size_t Bits, CRC_PARM_TYPE TrPo, CRC_PARM_TYPE InRe,
           CRC_PARM_TYPE FiXo, bool ReIn, bool ReRe >
void
PRIVATE_TESTER_NAME::master_test
(
    char const *                              test_name,
    typename PRIVATE_TESTER_NAME::value_type  expected
)
{
    std::cout << "Doing test suite for " << test_name << '.' << std::endl;
    compute_test( expected );
    interrupt_test( expected );
    error_test();
}

// Undo limited macros
#undef PRIVATE_TESTER_NAME


// A CRC-32 computer based on crc_basic, for timing
boost::uint32_t
basic_crc32
(
    void const *  buffer,
    std::size_t   byte_count
)
{
    static  boost::crc_basic<32>  computer( 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF,
     true, true );

    computer.reset();
    computer.process_bytes( buffer, byte_count );
    return computer.checksum();
}

// A CRC-32 computer based on crc_optimal, for timing
inline
boost::uint32_t
optimal_crc32
(
    void const *  buffer,
    std::size_t   byte_count
)
{
    static  boost::crc_32_type  computer;

    computer.reset();
    computer.process_bytes( buffer, byte_count );
    return computer.checksum();
}

// Reflect the lower "bits" bits of a "value"
boost::uint32_t
quick_reflect
(
    boost::uint32_t  value,
    std::size_t      bits
)
{
    boost::uint32_t  reflection = 0;
    for ( std::size_t i = 0 ; i < bits ; ++i )
    {
        if ( value & (1u << i) )
        {
            reflection |= 1 << ( bits - 1 - i );
        }
    }

    return reflection;
}

// A customized CRC-32 computer, for timing
boost::uint32_t
quick_crc32
(
    void const *  buffer,
    std::size_t   byte_count
)
{
    PRIVATE_DECLARE_BOOST( uint32_t );
    typedef unsigned char  byte_type;

    // Compute the CRC table (first run only)
    static  bool      did_init = false;
    static  uint32_t  crc_table[ 1ul << CHAR_BIT ];
    if ( !did_init )
    {
        uint32_t const  value_high_bit = static_cast<uint32_t>(1) << 31u;

        byte_type  dividend = 0;
        do
        {
            uint32_t  remainder = 0;
            for ( byte_type mask = 1u << (CHAR_BIT - 1u) ; mask ; mask >>= 1 )
            {
                if ( dividend & mask )
                {
                    remainder ^= value_high_bit;
                }

                if ( remainder & value_high_bit )
                {
                    remainder <<= 1;
                    remainder ^= 0x04C11DB7u;
                }
                else
                {
                    remainder <<= 1;
                }
            }

            crc_table[ quick_reflect(dividend, CHAR_BIT) ]
             = quick_reflect( remainder, 32 );
        }
        while ( ++dividend );

        did_init = true;
    }

    // Compute the CRC of the data
    uint32_t  rem = 0xFFFFFFFF;

    byte_type const * const  b_begin = static_cast<byte_type const *>( buffer );
    byte_type const * const  b_end = b_begin + byte_count;
    for ( byte_type const *p = b_begin ; p < b_end ; ++p )
    {
        byte_type const  byte_index = *p ^ rem;
        rem >>= CHAR_BIT;
        rem ^= crc_table[ byte_index ];
    }

    return ~rem;
}

// Run an individual timing trial
double
time_trial
(
    char const *       name,
    boost::uint32_t  (*crc_func)(void const *, std::size_t),
    boost::uint32_t    expected,
    void const *       data,
    std::size_t        length
)
{
    PRIVATE_DECLARE_BOOST( uint32_t );
    using std::cout;

    // Limits of a trial
    static uint32_t const  max_count = 1L << 16;  // ~square-root of max
    static double const    max_time = 3.14159;    // easy as pi(e)

    // Mark the trial
    cout << '\t' << name << " CRC-32: ";

    // Trial loop
    uint32_t      trial_count = 0, wrong_count = 0;
    double        elapsed_time = 0.0;
    boost::timer  t;

    do
    {
        uint32_t const  scratch = (*crc_func)( data, length );

        if ( scratch != expected )
        {
            ++wrong_count;
        }
        elapsed_time = t.elapsed();
        ++trial_count;
    } while ( (trial_count < max_count) && (elapsed_time < max_time) );

    if ( wrong_count )
    {
        BOOST_ERROR( "at least one time trial didn't match expected" );
    }

    // Report results
    double const  rate = trial_count / elapsed_time;

    cout << trial_count << " runs, " << elapsed_time << " s, " << rate
     << " run/s" << std::endl;
    return rate;
}

// Time runs of Boost CRCs vs. a customized CRC function
void
timing_test
(
)
{
    PRIVATE_DECLARE_BOOST( uint32_t );
    using std::cout;
    using std::endl;

    cout << "Doing timing tests." << endl;

    // Create a random block of data
    boost::int32_t     ran_data[ 256 ];
    std::size_t const  ran_length = sizeof(ran_data) / sizeof(ran_data[0]);

    std::generate_n( ran_data, ran_length, boost::minstd_rand() );

    // Use the first runs as a check.  This gives a chance for first-
    // time static initialization to not interfere in the timings.
    uint32_t const  basic_result = basic_crc32( ran_data, sizeof(ran_data) );
    uint32_t const  optimal_result = optimal_crc32( ran_data, sizeof(ran_data) );
    uint32_t const  quick_result = quick_crc32( ran_data, sizeof(ran_data) );

    BOOST_CHECK( basic_result == optimal_result );
    BOOST_CHECK( optimal_result == quick_result );
    BOOST_CHECK( quick_result == basic_result );

    // Run trials
    double const  basic_rate = time_trial( "Boost-Basic", basic_crc32,
     basic_result, ran_data, sizeof(ran_data) );
    double const  optimal_rate = time_trial( "Boost-Optimal", optimal_crc32,
     optimal_result, ran_data, sizeof(ran_data) );
    double const  quick_rate = time_trial( "Reference", quick_crc32,
     quick_result, ran_data, sizeof(ran_data) );

    // Report results
    cout << "\tThe optimal Boost version is " << (quick_rate - optimal_rate)
     / quick_rate * 100.0 << "% slower than the reference version.\n";
    cout << "\tThe basic Boost version is " << (quick_rate - basic_rate)
     / quick_rate * 100.0 << "% slower than the reference version.\n";
    cout << "\tThe basic Boost version is " << (optimal_rate - basic_rate)
     / optimal_rate * 100.0 << "% slower than the optimal Boost version."
     << endl;
}


// Reformat an integer to the big-endian storage format
boost::uint32_t
native_to_big
(
    boost::uint32_t  x
)
{
    boost::uint32_t  temp;
    unsigned char *  tp = reinterpret_cast<unsigned char *>( &temp );

    for ( std::size_t i = sizeof(x) ; i > 0 ; --i )
    {
        tp[ i - 1 ] = static_cast<unsigned char>( x );
        x >>= CHAR_BIT;
    }

    return temp;
}

// Restore an integer from the big-endian storage format
boost::uint32_t
big_to_native
(
    boost::uint32_t  x
)
{
    boost::uint32_t  temp = 0;
    unsigned char *  xp = reinterpret_cast<unsigned char *>( &x );

    for ( std::size_t i = 0 ; i < sizeof(x) ; ++i )
    {
        temp <<= CHAR_BIT;
        temp |= xp[ i ];
    }

    return temp;
}

// Run tests on using CRCs on augmented messages
void
augmented_tests
(
)
{
    #define PRIVATE_ACRC_FUNC  boost::augmented_crc<32, 0x04C11DB7>

    using std::size_t;
    PRIVATE_DECLARE_BOOST( uint32_t );

    std::cout << "Doing CRC-augmented message tests." << std::endl;

    // Create a random block of data, with space for a CRC.
    uint32_t      ran_data[ 257 ];
    size_t const  ran_length = sizeof(ran_data) / sizeof(ran_data[0]);
    size_t const  data_length = ran_length - 1;

    std::generate_n( ran_data, data_length, boost::minstd_rand() );

    // When creating a CRC for an augmented message, use
    // zeros in the appended CRC spot for the first run.
    uint32_t &  ran_crc = ran_data[ data_length ];

    ran_crc = 0;

    // Compute the CRC with augmented-CRC computing function
    typedef boost::uint_t<32>::fast  return_type;

    ran_crc = PRIVATE_ACRC_FUNC( ran_data, sizeof(ran_data) );

    // With the appended CRC set, running the checksum again should get zero.
    // NOTE: CRC algorithm assumes numbers are in big-endian format
    ran_crc = native_to_big( ran_crc );

    uint32_t  ran_crc_check = PRIVATE_ACRC_FUNC( ran_data, sizeof(ran_data) );

    BOOST_CHECK( 0 == ran_crc_check );

    // Compare that result with other CRC computing functions
    // and classes, which don't accept augmented messages.
    typedef boost::crc_optimal<32, 0x04C11DB7>  fast_crc_type;
    typedef boost::crc_basic<32>                slow_crc_type;

    fast_crc_type   fast_tester;
    slow_crc_type   slow_tester( 0x04C11DB7 );
    size_t const    data_size = data_length * sizeof(ran_data[0]);
    uint32_t const  func_tester = boost::crc<32, 0x04C11DB7, 0, 0, false,
     false>( ran_data, data_size );

    fast_tester.process_bytes( ran_data, data_size );
    slow_tester.process_bytes( ran_data, data_size );
    BOOST_CHECK( fast_tester.checksum() == slow_tester.checksum() );
    ran_crc = big_to_native( ran_crc );
    BOOST_CHECK( fast_tester.checksum() == ran_crc );
    BOOST_CHECK( func_tester == ran_crc );

    // Do a single-bit error test
    ran_crc = native_to_big( ran_crc );
    ran_data[ ran_data[0] % ran_length ] ^= ( 1 << (ran_data[1] % 32) );
    ran_crc_check = PRIVATE_ACRC_FUNC( ran_data, sizeof(ran_data) );
    BOOST_CHECK( 0 != ran_crc_check );

    // Run a version of these tests with a nonzero initial remainder.
    uint32_t const  init_rem = ran_data[ ran_data[2] % ran_length ];

    ran_crc = 0;
    ran_crc = PRIVATE_ACRC_FUNC( ran_data, sizeof(ran_data), init_rem );

    // Have some fun by processing data in two steps.
    size_t const  mid_index = ran_length / 2;

    ran_crc = native_to_big( ran_crc );
    ran_crc_check = PRIVATE_ACRC_FUNC( ran_data, mid_index
     * sizeof(ran_data[0]), init_rem );
    ran_crc_check = PRIVATE_ACRC_FUNC( &ran_data[mid_index], sizeof(ran_data)
     - mid_index * sizeof(ran_data[0]), ran_crc_check );
    BOOST_CHECK( 0 == ran_crc_check );

    // This substep translates an augmented-CRC initial
    // remainder to an unaugmented-CRC initial remainder.
    uint32_t const  zero = 0;
    uint32_t const  new_init_rem = PRIVATE_ACRC_FUNC( &zero, sizeof(zero),
     init_rem );
    slow_crc_type   slow_tester2( 0x04C11DB7, new_init_rem );

    slow_tester2.process_bytes( ran_data, data_size );
    ran_crc = big_to_native( ran_crc );
    BOOST_CHECK( slow_tester2.checksum() == ran_crc );

    // Redo single-bit error test
    ran_data[ ran_data[3] % ran_length ] ^= ( 1 << (ran_data[4] % 32) );
    ran_crc_check = PRIVATE_ACRC_FUNC( ran_data, sizeof(ran_data), init_rem );
    BOOST_CHECK( 0 != ran_crc_check );

    #undef PRIVATE_ACRC_FUNC
}


// Run tests on CRCs below a byte in size (here, 3 bits)
void
small_crc_test1
(
)
{
    std::cout << "Doing short-CRC (3-bit augmented) message tests."
     << std::endl;

    // The CRC standard is a SDH/SONET Low Order LCAS control word with CRC-3
    // taken from ITU-T G.707 (12/03) XIII.2.

    // Four samples, each four bytes; should all have a CRC of zero
    unsigned char const  samples[4][4]
      = {
            { 0x3A, 0xC4, 0x08, 0x06 },
            { 0x42, 0xC5, 0x0A, 0x41 },
            { 0x4A, 0xC5, 0x08, 0x22 },
            { 0x52, 0xC4, 0x08, 0x05 }
        };

    // Basic computer
    boost::crc_basic<3>  tester1( 0x03 );

    tester1.process_bytes( samples[0], 4 );
    BOOST_CHECK( tester1.checksum() == 0 );

    tester1.reset();
    tester1.process_bytes( samples[1], 4 );
    BOOST_CHECK( tester1.checksum() == 0 );

    tester1.reset();
    tester1.process_bytes( samples[2], 4 );
    BOOST_CHECK( tester1.checksum() == 0 );

    tester1.reset();
    tester1.process_bytes( samples[3], 4 );
    BOOST_CHECK( tester1.checksum() == 0 );

    // Optimal computer
    #define PRIVATE_CRC_FUNC   boost::crc<3, 0x03, 0, 0, false, false>
    #define PRIVATE_ACRC_FUNC  boost::augmented_crc<3, 0x03>

    BOOST_CHECK( 0 == PRIVATE_CRC_FUNC(samples[0], 4) );
    BOOST_CHECK( 0 == PRIVATE_CRC_FUNC(samples[1], 4) );
    BOOST_CHECK( 0 == PRIVATE_CRC_FUNC(samples[2], 4) );
    BOOST_CHECK( 0 == PRIVATE_CRC_FUNC(samples[3], 4) );

    // maybe the fix to CRC functions needs to be applied to augmented CRCs?

    #undef PRIVATE_ACRC_FUNC
    #undef PRIVATE_CRC_FUNC
}

// Run tests on CRCs below a byte in size (here, 7 bits)
void
small_crc_test2
(
)
{
    std::cout << "Doing short-CRC (7-bit augmented) message tests."
     << std::endl;

    // The CRC standard is a SDH/SONET J0/J1/J2/N1/N2/TR TTI (trace message)
    // with CRC-7, o.a. ITU-T G.707 Annex B, G.832 Annex A.

    // Two samples, each sixteen bytes
    // Sample 1 is '\x80' + ASCII("123456789ABCDEF")
    // Sample 2 is '\x80' + ASCII("TTI UNAVAILABLE")
    unsigned char const  samples[2][16]
      = {
            { 0x80, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41,
              0x42, 0x43, 0x44, 0x45, 0x46 },
            { 0x80, 0x54, 0x54, 0x49, 0x20, 0x55, 0x4E, 0x41, 0x56, 0x41, 0x49,
              0x4C, 0x41, 0x42, 0x4C, 0x45 }
        };
    unsigned const       results[2] = { 0x62, 0x23 };

    // Basic computer
    boost::crc_basic<7>  tester1( 0x09 );

    tester1.process_bytes( samples[0], 16 );
    BOOST_CHECK( tester1.checksum() == results[0] );

    tester1.reset();
    tester1.process_bytes( samples[1], 16 );
    BOOST_CHECK( tester1.checksum() == results[1] );

    // Optimal computer
    #define PRIVATE_CRC_FUNC   boost::crc<7, 0x09, 0, 0, false, false>
    #define PRIVATE_ACRC_FUNC  boost::augmented_crc<7, 0x09>

    BOOST_CHECK( results[0] == PRIVATE_CRC_FUNC(samples[0], 16) );
    BOOST_CHECK( results[1] == PRIVATE_CRC_FUNC(samples[1], 16) );

    // maybe the fix to CRC functions needs to be applied to augmented CRCs?

    #undef PRIVATE_ACRC_FUNC
    #undef PRIVATE_CRC_FUNC
}


#ifndef BOOST_MSVC
// Explicit template instantiations
// (needed to fix a link error in Metrowerks CodeWarrior Pro 5.3)
template class crc_tester<16, 0x1021, 0xFFFF, 0, false, false>;
template class crc_tester<16, 0x8005, 0, 0, true, true>;
template class crc_tester<32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true>;
#endif

// Main testing function
int
test_main
(
    int         ,   // "argc" is unused
    char *      []  // "argv" is unused
)
{
    using std::cout;
    using std::endl;

    // Run simulations on some CRC types
    typedef crc_tester<16, 0x1021, 0xFFFF, 0, false, false>  crc_ccitt_tester;
    typedef crc_tester<16, 0x8005, 0, 0, true, true>         crc_16_tester;
    typedef crc_tester<32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true>
      crc_32_tester;

    crc_ccitt_tester::master_test( "CRC-CCITT", std_crc_ccitt_result );
    crc_16_tester::master_test( "CRC-16", std_crc_16_result );
    crc_32_tester::master_test( "CRC-32", std_crc_32_result );

    // Run a timing comparison test
    timing_test();

    // Test using augmented messages
    augmented_tests();

    // Test with CRC types smaller than a byte
    small_crc_test1();
    small_crc_test2();

    // Try a CRC based on the (x + 1) polynominal, which is a factor in
    // many real-life polynominals and doesn't fit evenly in a byte.
    cout << "Doing one-bit polynominal CRC test." << endl;
    boost::crc_basic<1>  crc_1( 1 );
    crc_1.process_bytes( std_data, std_data_len );
    BOOST_CHECK( crc_1.checksum() == 1 );

    // Test the function object interface
    cout << "Doing functional object interface test." << endl;
    boost::crc_optimal<16, 0x8005, 0, 0, true, true>  crc_16;
    crc_16 = std::for_each( std_data, std_data + std_data_len, crc_16 );
    BOOST_CHECK( crc_16() == std_crc_16_result );

    return boost::exit_success;
}
