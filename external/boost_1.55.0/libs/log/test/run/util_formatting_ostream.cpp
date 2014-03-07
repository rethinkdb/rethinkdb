/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   util_formatting_ostream.cpp
 * \author Andrey Semashev
 * \date   26.05.2013
 *
 * \brief  This header contains tests for the formatting output stream wrapper.
 */

#define BOOST_TEST_MODULE util_formatting_ostream

#include <locale>
#include <string>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include "char_definitions.hpp"

#if defined(BOOST_LOG_USE_CHAR) && defined(BOOST_LOG_USE_WCHAR_T)

#define BOOST_UTF8_DECL
#define BOOST_UTF8_BEGIN_NAMESPACE namespace {
#define BOOST_UTF8_END_NAMESPACE }

#include <boost/detail/utf8_codecvt_facet.hpp>
#include <boost/detail/utf8_codecvt_facet.ipp>

#endif // defined(BOOST_LOG_USE_CHAR) && defined(BOOST_LOG_USE_WCHAR_T)

namespace logging = boost::log;

namespace {

template< typename CharT >
struct test_impl
{
    typedef CharT char_type;
    typedef test_data< char_type > strings;
    typedef std::basic_string< char_type > string_type;
    typedef std::basic_ostringstream< char_type > ostream_type;
    typedef logging::basic_formatting_ostream< char_type > formatting_ostream_type;

    template< typename StringT >
    static void width_formatting()
    {
        // Check that widening works
        {
            string_type str_fmt;
            formatting_ostream_type strm_fmt(str_fmt);
            strm_fmt << strings::abc() << std::setw(8) << (StringT)strings::abcd() << strings::ABC();
            strm_fmt.flush();

            ostream_type strm_correct;
            strm_correct << strings::abc() << std::setw(8) << (StringT)strings::abcd() << strings::ABC();

            BOOST_CHECK(equal_strings(strm_fmt.str(), strm_correct.str()));
        }

        // Check that the string is not truncated
        {
            string_type str_fmt;
            formatting_ostream_type strm_fmt(str_fmt);
            strm_fmt << strings::abc() << std::setw(1) << (StringT)strings::abcd() << strings::ABC();
            strm_fmt.flush();

            ostream_type strm_correct;
            strm_correct << strings::abc() << std::setw(1) << (StringT)strings::abcd() << strings::ABC();

            BOOST_CHECK(equal_strings(strm_fmt.str(), strm_correct.str()));
        }
    }

    template< typename StringT >
    static void fill_formatting()
    {
        string_type str_fmt;
        formatting_ostream_type strm_fmt(str_fmt);
        strm_fmt << strings::abc() << std::setfill(static_cast< char_type >('x')) << std::setw(8) << (StringT)strings::abcd() << strings::ABC();
        strm_fmt.flush();

        ostream_type strm_correct;
        strm_correct << strings::abc() << std::setfill(static_cast< char_type >('x')) << std::setw(8) << (StringT)strings::abcd() << strings::ABC();

        BOOST_CHECK(equal_strings(strm_fmt.str(), strm_correct.str()));
    }

    template< typename StringT >
    static void alignment()
    {
        // Left alignment
        {
            string_type str_fmt;
            formatting_ostream_type strm_fmt(str_fmt);
            strm_fmt << strings::abc() << std::setw(8) << std::left << (StringT)strings::abcd() << strings::ABC();
            strm_fmt.flush();

            ostream_type strm_correct;
            strm_correct << strings::abc() << std::setw(8) << std::left << (StringT)strings::abcd() << strings::ABC();

            BOOST_CHECK(equal_strings(strm_fmt.str(), strm_correct.str()));
        }

        // Right alignment
        {
            string_type str_fmt;
            formatting_ostream_type strm_fmt(str_fmt);
            strm_fmt << strings::abc() << std::setw(8) << std::right << (StringT)strings::abcd() << strings::ABC();
            strm_fmt.flush();

            ostream_type strm_correct;
            strm_correct << strings::abc() << std::setw(8) << std::right << (StringT)strings::abcd() << strings::ABC();

            BOOST_CHECK(equal_strings(strm_fmt.str(), strm_correct.str()));
        }
    }
};

} // namespace

// Test support for width formatting
BOOST_AUTO_TEST_CASE_TEMPLATE(width_formatting, CharT, char_types)
{
    typedef test_impl< CharT > test;
    test::BOOST_NESTED_TEMPLATE width_formatting< const CharT* >();
    test::BOOST_NESTED_TEMPLATE width_formatting< typename test::string_type >();
    test::BOOST_NESTED_TEMPLATE width_formatting< boost::basic_string_ref< CharT > >();
}

// Test support for filler character setup
BOOST_AUTO_TEST_CASE_TEMPLATE(fill_formatting, CharT, char_types)
{
    typedef test_impl< CharT > test;
    test::BOOST_NESTED_TEMPLATE fill_formatting< const CharT* >();
    test::BOOST_NESTED_TEMPLATE fill_formatting< typename test::string_type >();
    test::BOOST_NESTED_TEMPLATE fill_formatting< boost::basic_string_ref< CharT > >();
}

// Test support for text alignment
BOOST_AUTO_TEST_CASE_TEMPLATE(alignment, CharT, char_types)
{
    typedef test_impl< CharT > test;
    test::BOOST_NESTED_TEMPLATE alignment< const CharT* >();
    test::BOOST_NESTED_TEMPLATE alignment< typename test::string_type >();
    test::BOOST_NESTED_TEMPLATE alignment< boost::basic_string_ref< CharT > >();
}

#if defined(BOOST_LOG_USE_CHAR) && defined(BOOST_LOG_USE_WCHAR_T)

namespace {

const char narrow_chars[] =
{
    static_cast< char >(0xd0), static_cast< char >(0x9f), static_cast< char >(0xd1), static_cast< char >(0x80),
    static_cast< char >(0xd0), static_cast< char >(0xb8), static_cast< char >(0xd0), static_cast< char >(0xb2),
    static_cast< char >(0xd0), static_cast< char >(0xb5), static_cast< char >(0xd1), static_cast< char >(0x82),
    ',', ' ',
    static_cast< char >(0xd0), static_cast< char >(0xbc), static_cast< char >(0xd0), static_cast< char >(0xb8),
    static_cast< char >(0xd1), static_cast< char >(0x80), '!', 0
};
const wchar_t wide_chars[] = { 0x041f, 0x0440, 0x0438, 0x0432, 0x0435, 0x0442, L',', L' ', 0x043c, 0x0438, 0x0440, L'!', 0 };

template< typename StringT >
void test_narrowing_code_conversion()
{
    std::locale loc(std::locale::classic(), new utf8_codecvt_facet());

    std::string str_fmt;
    logging::formatting_ostream strm_fmt(str_fmt);
    strm_fmt.imbue(loc);
    strm_fmt << (StringT)wide_chars;
    strm_fmt.flush();

    BOOST_CHECK(equal_strings(str_fmt, std::string(narrow_chars)));
}

template< typename StringT >
void test_widening_code_conversion()
{
    std::locale loc(std::locale::classic(), new utf8_codecvt_facet());

    std::wstring str_fmt;
    logging::wformatting_ostream strm_fmt(str_fmt);
    strm_fmt.imbue(loc);
    strm_fmt << (StringT)narrow_chars;
    strm_fmt.flush();

    BOOST_CHECK(equal_strings(str_fmt, std::wstring(wide_chars)));
}

} // namespace

// Test character code conversion
BOOST_AUTO_TEST_CASE(character_code_conversion)
{
    test_narrowing_code_conversion< const wchar_t* >();
    test_widening_code_conversion< const char* >();
    test_narrowing_code_conversion< std::wstring >();
    test_widening_code_conversion< std::string >();
    test_narrowing_code_conversion< boost::wstring_ref >();
    test_widening_code_conversion< boost::string_ref >();
}

#endif
