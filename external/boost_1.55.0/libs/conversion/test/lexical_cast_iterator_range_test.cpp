//  Unit test for boost::lexical_cast.
//
//  See http://www.boost.org for most recent version, including documentation.
//
//  Copyright Antony Polukhin, 2012.
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

#include <boost/test/unit_test.hpp>
#include <boost/range/iterator_range.hpp>

using namespace boost;

#if defined(BOOST_NO_STRINGSTREAM) || defined(BOOST_NO_STD_WSTRING)
#define BOOST_LCAST_NO_WCHAR_T
#endif


#if !defined(BOOST_NO_CXX11_CHAR16_T) && !defined(BOOST_NO_CXX11_UNICODE_LITERALS) && !defined(_LIBCPP_VERSION)
#define BOOST_LC_RUNU16
#endif

#if !defined(BOOST_NO_CXX11_CHAR32_T) && !defined(BOOST_NO_CXX11_UNICODE_LITERALS) && !defined(_LIBCPP_VERSION)
#define BOOST_LC_RUNU32
#endif

struct class_with_user_defined_sream_operators {
    int i;

    operator int() const {
        return i;
    }
};

template <class CharT>
inline std::basic_istream<CharT>& operator >> (std::basic_istream<CharT>& istr, class_with_user_defined_sream_operators& rhs)
{
    return istr >> rhs.i;
}


template <class RngT>
void do_test_iterator_range_impl(const RngT& rng)
{
    BOOST_CHECK_EQUAL(lexical_cast<int>(rng), 1);
    BOOST_CHECK_EQUAL(lexical_cast<int>(rng.begin(), rng.size()), 1);
    BOOST_CHECK_EQUAL(lexical_cast<unsigned int>(rng), 1u);
    BOOST_CHECK_EQUAL(lexical_cast<unsigned int>(rng.begin(), rng.size()), 1u);
    BOOST_CHECK_EQUAL(lexical_cast<short>(rng), 1);
    BOOST_CHECK_EQUAL(lexical_cast<short>(rng.begin(), rng.size()), 1);
    BOOST_CHECK_EQUAL(lexical_cast<unsigned short>(rng), 1u);
    BOOST_CHECK_EQUAL(lexical_cast<unsigned short>(rng.begin(), rng.size()), 1u);
    BOOST_CHECK_EQUAL(lexical_cast<long int>(rng), 1);
    BOOST_CHECK_EQUAL(lexical_cast<long int>(rng.begin(), rng.size()), 1);
    BOOST_CHECK_EQUAL(lexical_cast<unsigned long int>(rng), 1u);
    BOOST_CHECK_EQUAL(lexical_cast<unsigned long int>(rng.begin(), rng.size()), 1u);

#ifdef BOOST_STL_SUPPORTS_NEW_UNICODE_LOCALES
    BOOST_CHECK_EQUAL(lexical_cast<float>(rng), 1.0f);
    BOOST_CHECK_EQUAL(lexical_cast<float>(rng.begin(), rng.size()), 1.0f);
    BOOST_CHECK_EQUAL(lexical_cast<double>(rng), 1.0);
    BOOST_CHECK_EQUAL(lexical_cast<double>(rng.begin(), rng.size()), 1.0);
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
    BOOST_CHECK_EQUAL(lexical_cast<long double>(rng), 1.0L);
    BOOST_CHECK_EQUAL(lexical_cast<long double>(rng.begin(), rng.size()), 1.0L);
#endif
    BOOST_CHECK_EQUAL(lexical_cast<class_with_user_defined_sream_operators>(rng), 1);
#endif
#if defined(BOOST_HAS_LONG_LONG)
    BOOST_CHECK_EQUAL(lexical_cast<boost::ulong_long_type>(rng), 1u);
    BOOST_CHECK_EQUAL(lexical_cast<boost::ulong_long_type>(rng.begin(), rng.size()), 1u);
    BOOST_CHECK_EQUAL(lexical_cast<boost::long_long_type>(rng), 1);
    BOOST_CHECK_EQUAL(lexical_cast<boost::long_long_type>(rng.begin(), rng.size()), 1);
#elif defined(BOOST_HAS_MS_INT64)
    BOOST_CHECK_EQUAL(lexical_cast<unsigned __int64>(rng), 1u);
    BOOST_CHECK_EQUAL(lexical_cast<unsigned __int64>(rng.begin(), rng.size()), 1u);
    BOOST_CHECK_EQUAL(lexical_cast<__int64>(rng), 1);
    BOOST_CHECK_EQUAL(lexical_cast<__int64>(rng.begin(), rng.size()), 1);
#endif
}

template <class CharT>
void test_it_range_using_any_chars(CharT* one, CharT* eleven)
{
    typedef CharT test_char_type;

    // Zero terminated
    iterator_range<test_char_type*> rng1(one, one + 1);
    do_test_iterator_range_impl(rng1);

    iterator_range<const test_char_type*> crng1(one, one + 1);
    do_test_iterator_range_impl(crng1);

    // Non zero terminated
    iterator_range<test_char_type*> rng2(eleven, eleven + 1);
    do_test_iterator_range_impl(rng2);

    iterator_range<const test_char_type*> crng2(eleven, eleven + 1);
    do_test_iterator_range_impl(crng2);
}

template <class CharT>
void test_it_range_using_char(CharT* one, CharT* eleven)
{
    typedef CharT test_char_type;

    iterator_range<test_char_type*> rng1(one, one + 1);
    BOOST_CHECK_EQUAL(lexical_cast<std::string>(rng1), "1");

    iterator_range<const test_char_type*> crng1(one, one + 1);
    BOOST_CHECK_EQUAL(lexical_cast<std::string>(crng1), "1");

    iterator_range<test_char_type*> rng2(eleven, eleven + 1);
    BOOST_CHECK_EQUAL(lexical_cast<std::string>(rng2), "1");

    iterator_range<const test_char_type*> crng2(eleven, eleven + 1);
    BOOST_CHECK_EQUAL(lexical_cast<std::string>(crng2), "1");

    BOOST_CHECK_EQUAL(lexical_cast<float>(rng1), 1.0f);
    BOOST_CHECK_EQUAL(lexical_cast<double>(rng1), 1.0);
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
    BOOST_CHECK_EQUAL(lexical_cast<long double>(rng1), 1.0L);
#endif
    BOOST_CHECK_EQUAL(lexical_cast<class_with_user_defined_sream_operators>(rng1), 1);

    BOOST_CHECK_EQUAL(lexical_cast<float>(crng2), 1.0f);
    BOOST_CHECK_EQUAL(lexical_cast<double>(crng2), 1.0);
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
    BOOST_CHECK_EQUAL(lexical_cast<long double>(crng2), 1.0L);
#endif
    BOOST_CHECK_EQUAL(lexical_cast<class_with_user_defined_sream_operators>(crng2), 1);

#ifndef BOOST_LCAST_NO_WCHAR_T
    BOOST_CHECK(lexical_cast<std::wstring>(rng1) == L"1");
    BOOST_CHECK(lexical_cast<std::wstring>(crng1) == L"1");
    BOOST_CHECK(lexical_cast<std::wstring>(rng2) == L"1");
    BOOST_CHECK(lexical_cast<std::wstring>(crng2) == L"1");
#endif

#if defined(BOOST_LC_RUNU16) && defined(BOOST_STL_SUPPORTS_NEW_UNICODE_LOCALES)
    typedef std::basic_string<char16_t> my_char16_string;
    BOOST_CHECK(lexical_cast<my_char16_string>(rng1) == u"1");
    BOOST_CHECK(lexical_cast<my_char16_string>(crng1) == u"1");
    BOOST_CHECK(lexical_cast<my_char16_string>(rng2) == u"1");
    BOOST_CHECK(lexical_cast<my_char16_string>(crng2) == u"1");
#endif

#if defined(BOOST_LC_RUNU32) && defined(BOOST_STL_SUPPORTS_NEW_UNICODE_LOCALES)
    typedef std::basic_string<char32_t> my_char32_string;
    BOOST_CHECK(lexical_cast<my_char32_string>(rng1) == U"1");
    BOOST_CHECK(lexical_cast<my_char32_string>(crng1) == U"1");
    BOOST_CHECK(lexical_cast<my_char32_string>(rng2) == U"1");
    BOOST_CHECK(lexical_cast<my_char32_string>(crng2) == U"1");
#endif
}

void test_char_iterator_ranges()
{
    typedef char test_char_type;
    test_char_type data1[] = "1";
    test_char_type data2[] = "11";
    test_it_range_using_any_chars(data1, data2);
    test_it_range_using_char(data1, data2);
}



void test_unsigned_char_iterator_ranges()
{
    typedef unsigned char test_char_type;
    test_char_type data1[] = "1";
    test_char_type data2[] = "11";
    test_it_range_using_any_chars(data1, data2);
    test_it_range_using_char(data1, data2);
}

void test_signed_char_iterator_ranges()
{
    typedef signed char test_char_type;
    test_char_type data1[] = "1";
    test_char_type data2[] = "11";
    test_it_range_using_any_chars(data1, data2);
    test_it_range_using_char(data1, data2);
}

void test_wchar_iterator_ranges()
{
#ifndef BOOST_LCAST_NO_WCHAR_T
    typedef wchar_t test_char_type;
    test_char_type data1[] = L"1";
    test_char_type data2[] = L"11";
    test_it_range_using_any_chars(data1, data2);
#endif

    BOOST_CHECK(true);
}

void test_char16_iterator_ranges()
{
#if defined(BOOST_LC_RUNU16)
    typedef char16_t test_char_type;
    test_char_type data1[] = u"1";
    test_char_type data2[] = u"11";
    test_it_range_using_any_chars(data1, data2);
#endif

    BOOST_CHECK(true);
}

void test_char32_iterator_ranges()
{
#if defined(BOOST_LC_RUNU32)
    typedef char32_t test_char_type;
    test_char_type data1[] = U"1";
    test_char_type data2[] = U"11";
    test_it_range_using_any_chars(data1, data2);
#endif

    BOOST_CHECK(true);
}

unit_test::test_suite *init_unit_test_suite(int, char *[])
{
    unit_test::test_suite *suite = BOOST_TEST_SUITE("lexical_cast. Testing conversions using iterator_range<>");
    suite->add(BOOST_TEST_CASE(&test_char_iterator_ranges));
    suite->add(BOOST_TEST_CASE(&test_unsigned_char_iterator_ranges));
    suite->add(BOOST_TEST_CASE(&test_signed_char_iterator_ranges));
    suite->add(BOOST_TEST_CASE(&test_wchar_iterator_ranges));
    suite->add(BOOST_TEST_CASE(&test_char16_iterator_ranges));
    suite->add(BOOST_TEST_CASE(&test_char32_iterator_ranges));

    return suite;
}
