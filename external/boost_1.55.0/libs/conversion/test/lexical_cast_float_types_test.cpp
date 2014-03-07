//  Unit test for boost::lexical_cast.
//
//  See http://www.boost.org for most recent version, including documentation.
//
//  Copyright Antony Polukhin, 2011.
//
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).

#include <boost/config.hpp>

#if defined(__INTEL_COMPILER)
#pragma warning(disable: 193 383 488 981 1418 1419)
#elif defined(BOOST_MSVC)
#pragma warning(disable: 4097 4100 4121 4127 4146 4244 4245 4511 4512 4701 4800)
#endif

#include <boost/lexical_cast.hpp>

#include <boost/cstdint.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

void test_conversion_from_to_float();
void test_conversion_from_to_double();
void test_conversion_from_to_long_double();

using namespace boost;


unit_test::test_suite *init_unit_test_suite(int, char *[])
{
    unit_test::test_suite *suite =
        BOOST_TEST_SUITE("lexical_cast float types unit test");
    suite->add(BOOST_TEST_CASE(&test_conversion_from_to_float));
    suite->add(BOOST_TEST_CASE(&test_conversion_from_to_double));
    suite->add(BOOST_TEST_CASE(&test_conversion_from_to_long_double));

    return suite;
}


// Replace "-,999" with "-999".
template<class CharT>
std::basic_string<CharT> to_str_gcc_workaround(std::basic_string<CharT> str)
{
    std::locale loc;
    std::numpunct<CharT> const& np = BOOST_USE_FACET(std::numpunct<CharT>, loc);
    std::ctype<CharT> const& ct = BOOST_USE_FACET(std::ctype<CharT>, loc);

    if(np.grouping().empty())
        return str;

    CharT prefix[3] = { ct.widen('-'), np.thousands_sep(), CharT() };

    if(str.find(prefix) != 0)
        return str;

    prefix[1] = CharT();
    str.replace(0, 2, prefix);
    return str;
}

template<class CharT, class T>
std::basic_string<CharT> to_str(T t)
{
    std::basic_ostringstream<CharT> o;
    o << t;
    return to_str_gcc_workaround(o.str());
}


template<class T>
void test_conversion_from_to_float_for_locale()
{
    std::locale current_locale;
    typedef std::numpunct<char> numpunct;
    numpunct const& np = BOOST_USE_FACET(numpunct, current_locale);
    if ( !np.grouping().empty() )
    {
        BOOST_CHECK_THROW(
                lexical_cast<T>( std::string("100") + np.thousands_sep() + np.thousands_sep() + "0" )
                , bad_lexical_cast);
        BOOST_CHECK_THROW(lexical_cast<T>( std::string("100") + np.thousands_sep() ), bad_lexical_cast);
        BOOST_CHECK_THROW(lexical_cast<T>( np.thousands_sep() + std::string("100") ), bad_lexical_cast);
        BOOST_CHECK_THROW(lexical_cast<T>( std::string("1") + np.thousands_sep() + np.decimal_point() + "e10" ), bad_lexical_cast);
        BOOST_CHECK_THROW(lexical_cast<T>( std::string("1e10") + np.thousands_sep() ), bad_lexical_cast);
        BOOST_CHECK_THROW(lexical_cast<T>( std::string("1") + np.thousands_sep() + "e10" ), bad_lexical_cast);

        BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>( to_str< char >(100000) ), 100000, (std::numeric_limits<T>::epsilon()) );
        BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>( to_str< char >(10000000u) ), 10000000u, (std::numeric_limits<T>::epsilon()) );
        BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>( to_str< char >(100) ), 100, (std::numeric_limits<T>::epsilon()) );
#if !defined(BOOST_LCAST_NO_WCHAR_T) && !defined(BOOST_NO_INTRINSIC_WCHAR_T)
        BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>( to_str< wchar_t >(100000) ), 100000, (std::numeric_limits<T>::epsilon()) );
        BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>( to_str< wchar_t >(10000000u) ), 10000000u, (std::numeric_limits<T>::epsilon()) );
        BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>( to_str< wchar_t >(100) ), 100, (std::numeric_limits<T>::epsilon()) );
#endif
        // Exception must not be thrown, when we are using no separators at all
        BOOST_CHECK_CLOSE_FRACTION( lexical_cast<T>("30000"), static_cast<T>(30000), (std::numeric_limits<T>::epsilon()) );
    }
}




/*
 * Converts char* [and wchar_t*] to float number type and checks, that generated
 * number does not exceeds allowed epsilon.
 */
#ifndef BOOST_LCAST_NO_WCHAR_T
#define CHECK_CLOSE_ABS_DIFF(VAL,PREFIX)                                                          \
    converted_val = lexical_cast<test_t>(#VAL);                                                   \
    BOOST_CHECK_CLOSE_FRACTION( (VAL ## L? VAL ## L : std::numeric_limits<test_t>::epsilon()),    \
                       (converted_val ? converted_val : std::numeric_limits<test_t>::epsilon()),  \
                       std::numeric_limits<test_t>::epsilon()                                     \
                     );                                                                           \
    BOOST_CHECK_EQUAL(converted_val, lexical_cast<test_t>(L## #VAL) );

#else
#define CHECK_CLOSE_ABS_DIFF(VAL,TYPE)                                                            \
    converted_val = lexical_cast<test_t>(#VAL);                                                   \
    BOOST_CHECK_CLOSE_FRACTION( (VAL ## L? VAL ## L : std::numeric_limits<test_t>::epsilon()),    \
                       (converted_val ? converted_val : std::numeric_limits<test_t>::epsilon()),  \
                       std::numeric_limits<test_t>::epsilon()                                     \
                     );
#endif

template <class TestType>
void test_converion_to_float_types()
{
    typedef TestType test_t;
    test_t converted_val;

    BOOST_CHECK_CLOSE_FRACTION(1.0, lexical_cast<test_t>('1'), (std::numeric_limits<test_t>::epsilon()));
    BOOST_CHECK_EQUAL(0.0, lexical_cast<test_t>('0'));

    unsigned char const uc_one = '1';
    unsigned char const uc_zero ='0';
    BOOST_CHECK_CLOSE_FRACTION(1.0, lexical_cast<test_t>(uc_one), (std::numeric_limits<test_t>::epsilon()));
    BOOST_CHECK_EQUAL(0.0, lexical_cast<test_t>(uc_zero));

    signed char const sc_one = '1';
    signed char const sc_zero ='0';
    BOOST_CHECK_CLOSE_FRACTION(1.0, lexical_cast<test_t>(sc_one), (std::numeric_limits<test_t>::epsilon()));
    BOOST_CHECK_EQUAL(0.0, lexical_cast<test_t>(sc_zero));

    BOOST_CHECK_CLOSE_FRACTION(1e34L, lexical_cast<test_t>( "10000000000000000000000000000000000"), (std::numeric_limits<test_t>::epsilon()) );

//    VC failes the next test
//    BOOST_CHECK_CLOSE_FRACTION(1e-35L, lexical_cast<test_t>("0.00000000000000000000000000000000001"), (std::numeric_limits<test_t>::epsilon()) );
    BOOST_CHECK_CLOSE_FRACTION(
                                    0.1111111111111111111111111111111111111111111111111111111111111111111111111L
            , lexical_cast<test_t>("0.1111111111111111111111111111111111111111111111111111111111111111111111111")
            , (std::numeric_limits<test_t>::epsilon()) );

    CHECK_CLOSE_ABS_DIFF(1,test_t);
    BOOST_CHECK_EQUAL(0,lexical_cast<test_t>("0"));
    CHECK_CLOSE_ABS_DIFF(-1,test_t);

    CHECK_CLOSE_ABS_DIFF(1.0, test_t);
    CHECK_CLOSE_ABS_DIFF(0.0, test_t);
    CHECK_CLOSE_ABS_DIFF(-1.0,test_t);

    CHECK_CLOSE_ABS_DIFF(1e1, test_t);
    CHECK_CLOSE_ABS_DIFF(0e1, test_t);
    CHECK_CLOSE_ABS_DIFF(-1e1,test_t);

    CHECK_CLOSE_ABS_DIFF(1.0e1, test_t);
    CHECK_CLOSE_ABS_DIFF(0.0e1, test_t);
    CHECK_CLOSE_ABS_DIFF(-1.0e1,test_t);

    CHECK_CLOSE_ABS_DIFF(1e-1, test_t);
    CHECK_CLOSE_ABS_DIFF(0e-1, test_t);
    CHECK_CLOSE_ABS_DIFF(-1e-1,test_t);

    CHECK_CLOSE_ABS_DIFF(1.0e-1, test_t);
    CHECK_CLOSE_ABS_DIFF(0.0e-1, test_t);
    CHECK_CLOSE_ABS_DIFF(-1.0e-1,test_t);

    CHECK_CLOSE_ABS_DIFF(1E1, test_t);
    CHECK_CLOSE_ABS_DIFF(0E1, test_t);
    CHECK_CLOSE_ABS_DIFF(-1E1,test_t);

    CHECK_CLOSE_ABS_DIFF(1.0E1, test_t);
    CHECK_CLOSE_ABS_DIFF(0.0E1, test_t);
    CHECK_CLOSE_ABS_DIFF(-1.0E1,test_t);

    CHECK_CLOSE_ABS_DIFF(1E-1, test_t);
    CHECK_CLOSE_ABS_DIFF(0E-1, test_t);
    CHECK_CLOSE_ABS_DIFF(-1E-1,test_t);

    CHECK_CLOSE_ABS_DIFF(1.0E-1, test_t);
    CHECK_CLOSE_ABS_DIFF(0.0E-1, test_t);
    CHECK_CLOSE_ABS_DIFF(-1.0E-1, test_t);

    CHECK_CLOSE_ABS_DIFF(.0E-1, test_t);
    CHECK_CLOSE_ABS_DIFF(.0E-1, test_t);
    CHECK_CLOSE_ABS_DIFF(-.0E-1, test_t);

    CHECK_CLOSE_ABS_DIFF(10.0, test_t);
    CHECK_CLOSE_ABS_DIFF(00.0, test_t);
    CHECK_CLOSE_ABS_DIFF(-10.0,test_t);

    CHECK_CLOSE_ABS_DIFF(10e1, test_t);
    CHECK_CLOSE_ABS_DIFF(00e1, test_t);
    CHECK_CLOSE_ABS_DIFF(-10e1,test_t);

    CHECK_CLOSE_ABS_DIFF(10.0e1, test_t);
    CHECK_CLOSE_ABS_DIFF(00.0e1, test_t);
    CHECK_CLOSE_ABS_DIFF(-10.0e1,test_t);

    CHECK_CLOSE_ABS_DIFF(10e-1, test_t);
    CHECK_CLOSE_ABS_DIFF(00e-1, test_t);
    CHECK_CLOSE_ABS_DIFF(-10e-1,test_t);

    CHECK_CLOSE_ABS_DIFF(10.0e-1, test_t);
    CHECK_CLOSE_ABS_DIFF(00.0e-1, test_t);
    CHECK_CLOSE_ABS_DIFF(-10.0e-1,test_t);

    CHECK_CLOSE_ABS_DIFF(10E1, test_t);
    CHECK_CLOSE_ABS_DIFF(00E1, test_t);
    CHECK_CLOSE_ABS_DIFF(-10E1,test_t);

    CHECK_CLOSE_ABS_DIFF(10.0E1, test_t);
    CHECK_CLOSE_ABS_DIFF(00.0E1, test_t);
    CHECK_CLOSE_ABS_DIFF(-10.0E1,test_t);

    CHECK_CLOSE_ABS_DIFF(10E-1, test_t);
    CHECK_CLOSE_ABS_DIFF(00E-1, test_t);
    CHECK_CLOSE_ABS_DIFF(-10E-1,test_t);

    CHECK_CLOSE_ABS_DIFF(10.0E-1, test_t);
    CHECK_CLOSE_ABS_DIFF(00.0E-1, test_t);
    CHECK_CLOSE_ABS_DIFF(-10.0E-1, test_t);

    CHECK_CLOSE_ABS_DIFF(-10101.0E-011, test_t);
    CHECK_CLOSE_ABS_DIFF(-10101093, test_t);
    CHECK_CLOSE_ABS_DIFF(10101093, test_t);

    CHECK_CLOSE_ABS_DIFF(-.34, test_t);
    CHECK_CLOSE_ABS_DIFF(.34, test_t);
    CHECK_CLOSE_ABS_DIFF(.34e10, test_t);

    BOOST_CHECK_THROW(lexical_cast<test_t>("-1.e"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("-1.E"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("1.e"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("1.E"), bad_lexical_cast);

    BOOST_CHECK_THROW(lexical_cast<test_t>("1.0e"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("1.0E"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("10E"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("10e"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("1.0e-"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("1.0E-"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("10E-"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("10e-"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("e1"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("e-1"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("e-"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>(".e"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>(".11111111111111111111111111111111111111111111111111111111111111111111ee"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>(".11111111111111111111111111111111111111111111111111111111111111111111e-"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("."), bad_lexical_cast);

    BOOST_CHECK_THROW(lexical_cast<test_t>("-B"), bad_lexical_cast);

    // Following two tests are not valid for C++11 compilers    
    //BOOST_CHECK_THROW(lexical_cast<test_t>("0xB"), bad_lexical_cast);
    //BOOST_CHECK_THROW(lexical_cast<test_t>("0x0"), bad_lexical_cast);

    BOOST_CHECK_THROW(lexical_cast<test_t>("--1.0"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("1.0e--1"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("1.0.0"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("1e1e1"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("1.0e-1e-1"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>(" 1.0"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("1.0 "), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>(""), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("-"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>('\0'), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>('-'), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>('.'), bad_lexical_cast);
}

template <class T>
void test_float_typess_for_overflows()
{
    typedef T test_t;
    test_t minvalue = (std::numeric_limits<test_t>::min)();
    std::string s_min_value = lexical_cast<std::string>(minvalue);
    BOOST_CHECK_CLOSE_FRACTION(minvalue, lexical_cast<test_t>(minvalue), (std::numeric_limits<test_t>::epsilon()));
    BOOST_CHECK_CLOSE_FRACTION(minvalue, lexical_cast<test_t>(s_min_value), (std::numeric_limits<test_t>::epsilon() * 2));

    test_t maxvalue = (std::numeric_limits<test_t>::max)();
    std::string s_max_value = lexical_cast<std::string>(maxvalue);
    BOOST_CHECK_CLOSE_FRACTION(maxvalue, lexical_cast<test_t>(maxvalue), (std::numeric_limits<test_t>::epsilon()));
    BOOST_CHECK_CLOSE_FRACTION(maxvalue, lexical_cast<test_t>(s_max_value), (std::numeric_limits<test_t>::epsilon()));

#ifndef _LIBCPP_VERSION
    // libc++ had a bug in implementation of stream conversions for values that must be represented as infinity. 
    // http://llvm.org/bugs/show_bug.cgi?id=15723#c4
    BOOST_CHECK_THROW(lexical_cast<test_t>(s_max_value+"1"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>(s_max_value+"9"), bad_lexical_cast);

    // VC9 can fail the following tests on floats and doubles when using stingstream...
    BOOST_CHECK_THROW(lexical_cast<test_t>("1"+s_max_value), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<test_t>("9"+s_max_value), bad_lexical_cast);
#endif

    if ( is_same<test_t,float>::value )
    {
        BOOST_CHECK_THROW(lexical_cast<test_t>( (std::numeric_limits<double>::max)() ), bad_lexical_cast);
        BOOST_CHECK(
                (std::numeric_limits<double>::min)() - std::numeric_limits<test_t>::epsilon()
                <= lexical_cast<test_t>( (std::numeric_limits<double>::min)() )
                && lexical_cast<test_t>( (std::numeric_limits<double>::min)() )
                <= (std::numeric_limits<double>::min)() + std::numeric_limits<test_t>::epsilon()
        );
    }

    if ( sizeof(test_t) < sizeof(long double) )
    {
        BOOST_CHECK_THROW(lexical_cast<test_t>( (std::numeric_limits<long double>::max)() ), bad_lexical_cast);
        BOOST_CHECK(
                (std::numeric_limits<long double>::min)() - std::numeric_limits<test_t>::epsilon()
                <= lexical_cast<test_t>( (std::numeric_limits<long double>::min)() )
                && lexical_cast<test_t>( (std::numeric_limits<long double>::min)() )
                <= (std::numeric_limits<long double>::min)() + std::numeric_limits<test_t>::epsilon()
        );
    }
}

#undef CHECK_CLOSE_ABS_DIFF

// Epsilon is multiplied by 2 because of two lexical conversions
#define TEST_TO_FROM_CAST_AROUND_TYPED(VAL,STRING_TYPE)                             \
    test_value = VAL + std::numeric_limits<test_t>::epsilon() * i ;                 \
    converted_val = lexical_cast<test_t>( lexical_cast<STRING_TYPE>(test_value) );  \
    BOOST_CHECK_CLOSE_FRACTION(                                                     \
            test_value,                                                             \
            converted_val,                                                          \
            std::numeric_limits<test_t>::epsilon() * 2                              \
        );

/*
 * For interval [ from_mult*epsilon+VAL, to_mult*epsilon+VAL ], converts float type
 * numbers to string[wstring] and then back to float type, then compares initial
 * values and generated.
 * Step is epsilon
 */
#ifndef BOOST_LCAST_NO_WCHAR_T
#   define TEST_TO_FROM_CAST_AROUND(VAL)                    \
    for(i=from_mult; i<=to_mult; ++i) {                     \
        TEST_TO_FROM_CAST_AROUND_TYPED(VAL, std::string)    \
        TEST_TO_FROM_CAST_AROUND_TYPED(VAL, std::wstring)   \
    }
#else
#   define TEST_TO_FROM_CAST_AROUND(VAL)                    \
    for(i=from_mult; i<=to_mult; ++i) {                     \
        TEST_TO_FROM_CAST_AROUND_TYPED(VAL, std::string)    \
    }
#endif

template <class TestType>
void test_converion_from_to_float_types()
{
    typedef TestType test_t;
    test_t test_value;
    test_t converted_val;

    int i;
    int from_mult = -50;
    int to_mult = 50;

    TEST_TO_FROM_CAST_AROUND( 0.0 );

    long double val1;
    for(val1 = 1.0e-10L; val1 < 1e11; val1*=10 )
        TEST_TO_FROM_CAST_AROUND( val1 );

    long double val2;
    for(val2 = -1.0e-10L; val2 > -1e11; val2*=10 )
        TEST_TO_FROM_CAST_AROUND( val2 );

    from_mult = -100;
    to_mult = 0;
    TEST_TO_FROM_CAST_AROUND( (std::numeric_limits<test_t>::max)() );

    from_mult = 0;
    to_mult = 100;
    TEST_TO_FROM_CAST_AROUND( (std::numeric_limits<test_t>::min)() );
}

#undef TEST_TO_FROM_CAST_AROUND
#undef TEST_TO_FROM_CAST_AROUND_TYPED


template<class T, class CharT>
void test_conversion_from_float_to_char(CharT zero)
{
    BOOST_CHECK(lexical_cast<CharT>(static_cast<T>(0)) == zero + 0);
    BOOST_CHECK(lexical_cast<CharT>(static_cast<T>(1)) == zero + 1);
    BOOST_CHECK(lexical_cast<CharT>(static_cast<T>(2)) == zero + 2);
    BOOST_CHECK(lexical_cast<CharT>(static_cast<T>(3)) == zero + 3);
    BOOST_CHECK(lexical_cast<CharT>(static_cast<T>(4)) == zero + 4);
    BOOST_CHECK(lexical_cast<CharT>(static_cast<T>(5)) == zero + 5);
    BOOST_CHECK(lexical_cast<CharT>(static_cast<T>(6)) == zero + 6);
    BOOST_CHECK(lexical_cast<CharT>(static_cast<T>(7)) == zero + 7);
    BOOST_CHECK(lexical_cast<CharT>(static_cast<T>(8)) == zero + 8);
    BOOST_CHECK(lexical_cast<CharT>(static_cast<T>(9)) == zero + 9);

    BOOST_CHECK_THROW(lexical_cast<CharT>(static_cast<T>(10)), bad_lexical_cast);

    T t = (std::numeric_limits<T>::max)();
    BOOST_CHECK_THROW(lexical_cast<CharT>(t), bad_lexical_cast);
}

template<class T, class CharT>
void test_conversion_from_char_to_float(CharT zero)
{
    BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>( static_cast<CharT>(zero + 0)), static_cast<T>(0), (std::numeric_limits<T>::epsilon()) );
    BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>( static_cast<CharT>(zero + 1)), static_cast<T>(1), (std::numeric_limits<T>::epsilon()) );
    BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>( static_cast<CharT>(zero + 2)), static_cast<T>(2), (std::numeric_limits<T>::epsilon()) );
    BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>( static_cast<CharT>(zero + 3)), static_cast<T>(3), (std::numeric_limits<T>::epsilon()) );
    BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>( static_cast<CharT>(zero + 4)), static_cast<T>(4), (std::numeric_limits<T>::epsilon()) );
    BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>( static_cast<CharT>(zero + 5)), static_cast<T>(5), (std::numeric_limits<T>::epsilon()) );
    BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>( static_cast<CharT>(zero + 6)), static_cast<T>(6), (std::numeric_limits<T>::epsilon()) );
    BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>( static_cast<CharT>(zero + 7)), static_cast<T>(7), (std::numeric_limits<T>::epsilon()) );
    BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>( static_cast<CharT>(zero + 8)), static_cast<T>(8), (std::numeric_limits<T>::epsilon()) );
    BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>( static_cast<CharT>(zero + 9)), static_cast<T>(9), (std::numeric_limits<T>::epsilon()) );

    BOOST_CHECK_THROW(lexical_cast<T>( static_cast<CharT>(zero + 10)), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<T>( static_cast<CharT>(zero - 1)), bad_lexical_cast);
}

struct restore_oldloc
{
    std::locale oldloc;
    ~restore_oldloc() { std::locale::global(oldloc); }
};

template<class T>
void test_conversion_from_to_float()
{    char const zero = '0';
    signed char const szero = '0';
    unsigned char const uzero = '0';
    test_conversion_from_float_to_char<T>(zero);
    test_conversion_from_char_to_float<T>(zero);
    test_conversion_from_float_to_char<T>(szero);
    test_conversion_from_char_to_float<T>(szero);
    test_conversion_from_float_to_char<T>(uzero);
    test_conversion_from_char_to_float<T>(uzero);
    #if !defined(BOOST_LCAST_NO_WCHAR_T) && !defined(BOOST_NO_INTRINSIC_WCHAR_T)
    wchar_t const wzero = L'0';
    test_conversion_from_float_to_char<T>(wzero);
    test_conversion_from_char_to_float<T>(wzero);
    #endif

    BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>("+1"), 1, std::numeric_limits<T>::epsilon());
    BOOST_CHECK_CLOSE_FRACTION(lexical_cast<T>("+9"), 9, std::numeric_limits<T>::epsilon());

    BOOST_CHECK_THROW(lexical_cast<T>("++1"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<T>("-+9"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<T>("--1"), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<T>("+-9"), bad_lexical_cast);

    test_converion_to_float_types<T>();
    test_float_typess_for_overflows<T>();
    test_converion_from_to_float_types<T>();


    typedef std::numpunct<char> numpunct;

    restore_oldloc guard;
    std::locale const& oldloc = guard.oldloc;

    std::string grouping1 = BOOST_USE_FACET(numpunct, oldloc).grouping();
    std::string grouping2(grouping1);

    test_conversion_from_to_float_for_locale<T>();

    try
    {
        std::locale newloc("");
        std::locale::global(newloc);

        grouping2 = BOOST_USE_FACET(numpunct, newloc).grouping();
    }
    catch(std::exception const& ex)
    {
        std::string msg("Failed to set system locale: ");
        msg += ex.what();
        BOOST_TEST_MESSAGE(msg);
    }

    if(grouping1 != grouping2)
        test_conversion_from_to_float_for_locale<T>();

    if(grouping1.empty() && grouping2.empty())
        BOOST_TEST_MESSAGE("Formatting with thousands_sep has not been tested");
}


void test_conversion_from_to_float()
{
    test_conversion_from_to_float<float>();
}
void test_conversion_from_to_double()
{
    test_conversion_from_to_float<double>();
}
void test_conversion_from_to_long_double()
{
// We do not run tests on compilers with bugs
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
    test_conversion_from_to_float<long double>();
#endif
    BOOST_CHECK(true);
}







