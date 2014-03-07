/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2001-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <iostream>
#include <climits>

using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

template <typename T>
struct ts_real_parser_policies : public ureal_parser_policies<T>
{
    //  These policies can be used to parse thousand separated
    //  numbers with at most 2 decimal digits after the decimal
    //  point. e.g. 123,456,789.01

    typedef uint_parser<int, 10, 1, 2>  uint2_t;
    typedef uint_parser<T, 10, 1, -1>   uint_parser_t;
    typedef int_parser<int, 10, 1, -1>  int_parser_t;

    //////////////////////////////////  2 decimal places Max
    template <typename ScannerT>
    static typename parser_result<uint2_t, ScannerT>::type
    parse_frac_n(ScannerT& scan)
    { return uint2_t().parse(scan); }

    //////////////////////////////////  No exponent
    template <typename ScannerT>
    static typename parser_result<chlit<>, ScannerT>::type
    parse_exp(ScannerT& scan)
    { return scan.no_match(); }

    //////////////////////////////////  No exponent
    template <typename ScannerT>
    static typename parser_result<int_parser_t, ScannerT>::type
    parse_exp_n(ScannerT& scan)
    { return scan.no_match(); }

    //////////////////////////////////  Thousands separated numbers
    template <typename ScannerT>
    static typename parser_result<uint_parser_t, ScannerT>::type
    parse_n(ScannerT& scan)
    {
        typedef typename parser_result<uint_parser_t, ScannerT>::type RT;
        static uint_parser<unsigned, 10, 1, 3> uint3_p;
        static uint_parser<unsigned, 10, 3, 3> uint3_3_p;

        if (RT hit = uint3_p.parse(scan))
        {
            T n;
            typedef typename ScannerT::iterator_t iterator_t;
            iterator_t save = scan.first;
            while (match<> next = (',' >> uint3_3_p[assign_a(n)]).parse(scan))
            {
                hit.value((hit.value() * 1000) + n);
                scan.concat_match(hit, next);
                save = scan.first;
            }
            scan.first = save;
            return hit;
        }
        return scan.no_match();
    }
};

real_parser<double, ts_real_parser_policies<double> > const
    ts_real_p = real_parser<double, ts_real_parser_policies<double> >();

template <typename T>
struct no_trailing_dot : public real_parser_policies<T>
{
    static const bool allow_trailing_dot = false;
};

real_parser<double, no_trailing_dot<double> > const
    notrdot_real_p = real_parser<double, no_trailing_dot<double> >();

template <typename T>
struct no_leading_dot : public real_parser_policies<T>
{
    static const bool allow_leading_dot = false;
};

real_parser<double, no_leading_dot<double> > const
    nolddot_real_p = real_parser<double, no_leading_dot<double> >();

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    cout << "/////////////////////////////////////////////////////////\n\n";
    cout << "\t\tNumeric tests...\n\n";
    cout << "/////////////////////////////////////////////////////////\n\n";

    //  *** The following assumes 32 bit integers. Modify these constant
    //  *** strings when appropriate. BEWARE PLATFORM DEPENDENT!

    char const* max_unsigned = "4294967295";
    char const* unsigned_overflow = "4294967296";
    char const* max_int = "2147483647";
    char const* int_overflow = "2147483648";
    char const* min_int = "-2147483648";
    char const* int_underflow = "-2147483649";
    char const* max_binary = "11111111111111111111111111111111";
    char const* binary_overflow = "100000000000000000000000000000000";
    char const* max_octal = "37777777777";
    char const* octal_overflow = "100000000000";
    char const* max_hex = "FFFFFFFF";
    char const* hex_overflow = "100000000";

#ifdef BOOST_HAS_LONG_LONG

    char const* max_long_long = "9223372036854775807";
    char const* long_long_overflow = "9223372036854775808";
    char const* min_long_long = "-9223372036854775808";
    char const* long_long_underflow = "-9223372036854775809";

#endif

//  BEGIN TESTS...

    unsigned u;

//  unsigned integer

    parse("123456", uint_p[assign_a(u)]);
    BOOST_TEST(u == 123456);

    parse(max_unsigned, uint_p[assign_a(u)]);
    BOOST_TEST(u == UINT_MAX);

    BOOST_TEST(!parse(unsigned_overflow, uint_p).full);

//  signed integer

    int i;

    parse("123456", int_p[assign_a(i)]);
    BOOST_TEST(i == 123456);

    parse("-123456", int_p[assign_a(i)]);
    BOOST_TEST(i == -123456);

    parse(max_int, int_p[assign_a(i)]);
    BOOST_TEST(i == INT_MAX);

    parse(min_int, int_p[assign_a(i)]);
    BOOST_TEST(i == INT_MIN);

    BOOST_TEST(!parse(int_overflow, int_p).full);
    BOOST_TEST(!parse(int_underflow, int_p).full);

    BOOST_TEST(!parse("-", int_p).hit);

//  binary

    parse("11111110", bin_p[assign_a(u)]);
    BOOST_TEST(u == 0xFE);

    parse(max_binary, bin_p[assign_a(u)]);
    BOOST_TEST(u == UINT_MAX);

    BOOST_TEST(!parse(binary_overflow, bin_p).full);

//  octal

    parse("12545674515", oct_p[assign_a(u)]);
    BOOST_TEST(u == 012545674515);

    parse(max_octal, oct_p[assign_a(u)]);
    BOOST_TEST(u == UINT_MAX);

    BOOST_TEST(!parse(octal_overflow, oct_p).full);

//  hex

    parse("95BC8DF", hex_p[assign_a(u)]);
    BOOST_TEST(u == 0x95BC8DF);

    parse("abcdef12", hex_p[assign_a(u)]);
    BOOST_TEST(u == 0xabcdef12);

    parse(max_hex, hex_p[assign_a(u)]);
    BOOST_TEST(u == UINT_MAX);

    BOOST_TEST(!parse(hex_overflow, hex_p).full);

//  limited fieldwidth

    uint_parser<unsigned, 10, 1, 3> uint3_p;
    parse("123456", uint3_p[assign_a(u)]);
    BOOST_TEST(u == 123);

    uint_parser<unsigned, 10, 1, 4> uint4_p;
    parse("123456", uint4_p[assign_a(u)]);
    BOOST_TEST(u == 1234);

    uint_parser<unsigned, 10, 3, 3> uint3_3_p;

//  thousand separated numbers

#define r (uint3_p >> *(',' >> uint3_3_p))

    BOOST_TEST(parse("1,234,567,890", r).full);     //  OK
    BOOST_TEST(parse("12,345,678,900", r).full);    //  OK
    BOOST_TEST(parse("123,456,789,000", r).full);   //  OK
    BOOST_TEST(!parse("1000,234,567,890", r).full); //  Bad
    BOOST_TEST(!parse("1,234,56,890", r).full);     //  Bad
    BOOST_TEST(!parse("1,66", r).full);             //  Bad

//  long long

#ifdef BOOST_HAS_LONG_LONG

// Some compilers have long long, but don't define the
// LONG_LONG_MIN and LONG_LONG_MAX macros in limits.h.  This
// assumes that long long is 64 bits.
#if !defined(LONG_LONG_MIN) && !defined(LONG_LONG_MAX) \
            && !defined(ULONG_LONG_MAX)
#define ULONG_LONG_MAX 0xffffffffffffffffLLU
#define LONG_LONG_MAX 0x7fffffffffffffffLL
#define LONG_LONG_MIN (-LONG_LONG_MAX - 1)
#endif

     ::boost::long_long_type ll;
    int_parser< ::boost::long_long_type> long_long_p;

    parse("1234567890123456789", long_long_p[assign_a(ll)]);
    BOOST_TEST(ll == 1234567890123456789LL);

    parse("-1234567890123456789", long_long_p[assign_a(ll)]);
    BOOST_TEST(ll == -1234567890123456789LL);

    parse(max_long_long, long_long_p[assign_a(ll)]);
    BOOST_TEST(ll == LONG_LONG_MAX);

    parse(min_long_long, long_long_p[assign_a(ll)]);
    BOOST_TEST(ll == LONG_LONG_MIN);

#if defined(__GNUG__) && (__GNUG__ == 3) && (__GNUC_MINOR__ < 3) \
    && !defined(__EDG__)
    // gcc 3.2.3 crashes on parse(long_long_overflow, long_long_p)
    // wrapping long_long_p into a rule avoids the crash
    rule<> gcc_3_2_3_long_long_r = long_long_p;
    BOOST_TEST(!parse(long_long_overflow, gcc_3_2_3_long_long_r).full);
    BOOST_TEST(!parse(long_long_underflow, gcc_3_2_3_long_long_r).full);
#else
    BOOST_TEST(!parse(long_long_overflow, long_long_p).full);
    BOOST_TEST(!parse(long_long_underflow, long_long_p).full);
#endif

#endif

//  real numbers

    double  d;

    BOOST_TEST(parse("1234", ureal_p[assign_a(d)]).full && d == 1234);      //  Good.
    BOOST_TEST(parse("1.2e3", ureal_p[assign_a(d)]).full && d == 1.2e3);    //  Good.
    BOOST_TEST(parse("1.2e-3", ureal_p[assign_a(d)]).full && d == 1.2e-3);  //  Good.
    BOOST_TEST(parse("1.e2", ureal_p[assign_a(d)]).full && d == 1.e2);      //  Good.
    BOOST_TEST(parse(".2e3", ureal_p[assign_a(d)]).full && d == .2e3);      //  Good.
    BOOST_TEST(parse("2e3", ureal_p[assign_a(d)]).full && d == 2e3);        //  Good. No fraction
    BOOST_TEST(!parse("e3", ureal_p).full);                                 //  Bad! No number
    BOOST_TEST(!parse("-1.2e3", ureal_p).full);                             //  Bad! Negative number
    BOOST_TEST(!parse("+1.2e3", ureal_p).full);                             //  Bad! Positive sign
    BOOST_TEST(!parse("1.2e", ureal_p).full);                               //  Bad! No exponent
    BOOST_TEST(!parse("-.3", ureal_p).full);                                //  Bad! Negative

    BOOST_TEST(parse("-1234", real_p[assign_a(d)]).full && d == -1234);     //  Good.
    BOOST_TEST(parse("-1.2e3", real_p[assign_a(d)]).full && d == -1.2e3);   //  Good.
    BOOST_TEST(parse("+1.2e3", real_p[assign_a(d)]).full && d == 1.2e3);    //  Good.
    BOOST_TEST(parse("-0.1", real_p[assign_a(d)]).full && d == -0.1);       //  Good.
    BOOST_TEST(parse("-1.2e-3", real_p[assign_a(d)]).full && d == -1.2e-3); //  Good.
    BOOST_TEST(parse("-1.e2", real_p[assign_a(d)]).full && d == -1.e2);     //  Good.
    BOOST_TEST(parse("-.2e3", real_p[assign_a(d)]).full && d == -.2e3);     //  Good.
    BOOST_TEST(parse("-2e3", real_p[assign_a(d)]).full && d == -2e3);       //  Good. No fraction
    BOOST_TEST(!parse("-e3", real_p).full);                                 //  Bad! No number
    BOOST_TEST(!parse("-1.2e", real_p).full);                               //  Bad! No exponent

    BOOST_TEST(!parse("1234", strict_ureal_p[assign_a(d)]).full);           //  Bad. Strict real
    BOOST_TEST(parse("1.2", strict_ureal_p[assign_a(d)]).full && d == 1.2); //  Good.
    BOOST_TEST(!parse("-1234", strict_real_p[assign_a(d)]).full);           //  Bad. Strict real
    BOOST_TEST(parse("123.", strict_real_p[assign_a(d)]).full && d == 123); //  Good.
    BOOST_TEST(parse("3.E6", strict_real_p[assign_a(d)]).full && d == 3e6); //  Good.

    BOOST_TEST(!parse("1234.", notrdot_real_p[assign_a(d)]).full);          //  Bad trailing dot
    BOOST_TEST(!parse(".1234", nolddot_real_p[assign_a(d)]).full);          //  Bad leading dot

//  Special thousands separated numbers

    BOOST_TEST(parse("123,456,789.01", ts_real_p[assign_a(d)]).full && d == 123456789.01);  //  Good.
    BOOST_TEST(parse("12,345,678.90", ts_real_p[assign_a(d)]).full && d == 12345678.90);    //  Good.
    BOOST_TEST(parse("1,234,567.89", ts_real_p[assign_a(d)]).full && d == 1234567.89);      //  Good.
    BOOST_TEST(!parse("1234,567,890", ts_real_p).full);     //  Bad.
    BOOST_TEST(!parse("1,234,5678,9", ts_real_p).full);     //  Bad.
    BOOST_TEST(!parse("1,234,567.89e6", ts_real_p).full);   //  Bad.
    BOOST_TEST(!parse("1,66", ts_real_p).full);             //  Bad.

//  END TESTS.

/////////////////////////////////////////////////////////////////
    return boost::report_errors();
}
