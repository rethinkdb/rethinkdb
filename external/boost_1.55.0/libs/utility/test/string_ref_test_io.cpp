/*
 *             Copyright Andrey Semashev 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   string_ref_test_io.cpp
 * \author Andrey Semashev
 * \date   26.05.2013
 *
 * \brief  This header contains tests for stream operations of \c basic_string_ref.
 */

#define BOOST_TEST_MODULE string_ref_test_io

#include <boost/utility/string_ref.hpp>

#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <string>

#include <boost/config.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/test/unit_test.hpp>

typedef boost::mpl::vector<
    char
#if !defined(BOOST_NO_STD_WSTRING) && !defined(BOOST_NO_STD_WSTREAMBUF) && !defined(BOOST_NO_INTRINSIC_WCHAR_T)
    , wchar_t
#endif
/* Current implementations seem to be missing codecvt facets to convert chars to char16_t and char32_t even though the types are available.
#if !defined(BOOST_NO_CXX11_CHAR16_T)
    , char16_t
#endif
#if !defined(BOOST_NO_CXX11_CHAR32_T)
    , char32_t
#endif
*/
>::type char_types;

static const char* test_strings[] =
{
    "begin",
    "abcd",
    "end"
};

//! The context with test data for particular character type
template< typename CharT >
struct context
{
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef std::basic_ostringstream< char_type > ostream_type;

    string_type begin, abcd, end;

    context()
    {
        boost::string_ref str = test_strings[0];
        std::copy(str.begin(), str.end(), std::back_inserter(begin));

        str = test_strings[1];
        std::copy(str.begin(), str.end(), std::back_inserter(abcd));

        str = test_strings[2];
        std::copy(str.begin(), str.end(), std::back_inserter(end));
    }
};

// Test regular output
BOOST_AUTO_TEST_CASE_TEMPLATE(string_ref_output, CharT, char_types)
{
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef std::basic_ostringstream< char_type > ostream_type;
    typedef boost::basic_string_ref< char_type > string_ref_type;

    context< char_type > ctx;

    ostream_type strm;
    strm << string_ref_type(ctx.abcd);
    BOOST_CHECK(strm.str() == ctx.abcd);
}

// Test support for padding
BOOST_AUTO_TEST_CASE_TEMPLATE(padding, CharT, char_types)
{
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef std::basic_ostringstream< char_type > ostream_type;
    typedef boost::basic_string_ref< char_type > string_ref_type;

    context< char_type > ctx;

    // Test for padding
    {
        ostream_type strm_ref;
        strm_ref << ctx.begin << std::setw(8) << string_ref_type(ctx.abcd) << ctx.end;

        ostream_type strm_correct;
        strm_correct << ctx.begin << std::setw(8) << ctx.abcd << ctx.end;

        BOOST_CHECK(strm_ref.str() == strm_correct.str());
    }

    // Test for long padding
    {
        ostream_type strm_ref;
        strm_ref << ctx.begin << std::setw(100) << string_ref_type(ctx.abcd) << ctx.end;

        ostream_type strm_correct;
        strm_correct << ctx.begin << std::setw(100) << ctx.abcd << ctx.end;

        BOOST_CHECK(strm_ref.str() == strm_correct.str());
    }

    // Test that short width does not truncate the string
    {
        ostream_type strm_ref;
        strm_ref << ctx.begin << std::setw(1) << string_ref_type(ctx.abcd) << ctx.end;

        ostream_type strm_correct;
        strm_correct << ctx.begin << std::setw(1) << ctx.abcd << ctx.end;

        BOOST_CHECK(strm_ref.str() == strm_correct.str());
    }
}

// Test support for padding fill
BOOST_AUTO_TEST_CASE_TEMPLATE(padding_fill, CharT, char_types)
{
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef std::basic_ostringstream< char_type > ostream_type;
    typedef boost::basic_string_ref< char_type > string_ref_type;

    context< char_type > ctx;

    ostream_type strm_ref;
    strm_ref << ctx.begin << std::setfill(static_cast< char_type >('x')) << std::setw(8) << string_ref_type(ctx.abcd) << ctx.end;

    ostream_type strm_correct;
    strm_correct << ctx.begin << std::setfill(static_cast< char_type >('x')) << std::setw(8) << ctx.abcd << ctx.end;

    BOOST_CHECK(strm_ref.str() == strm_correct.str());
}

// Test support for alignment
BOOST_AUTO_TEST_CASE_TEMPLATE(alignment, CharT, char_types)
{
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef std::basic_ostringstream< char_type > ostream_type;
    typedef boost::basic_string_ref< char_type > string_ref_type;

    context< char_type > ctx;

    // Left alignment
    {
        ostream_type strm_ref;
        strm_ref << ctx.begin << std::left << std::setw(8) << string_ref_type(ctx.abcd) << ctx.end;

        ostream_type strm_correct;
        strm_correct << ctx.begin << std::left << std::setw(8) << ctx.abcd << ctx.end;

        BOOST_CHECK(strm_ref.str() == strm_correct.str());
    }

    // Right alignment
    {
        ostream_type strm_ref;
        strm_ref << ctx.begin << std::right << std::setw(8) << string_ref_type(ctx.abcd) << ctx.end;

        ostream_type strm_correct;
        strm_correct << ctx.begin << std::right << std::setw(8) << ctx.abcd << ctx.end;

        BOOST_CHECK(strm_ref.str() == strm_correct.str());
    }
}
