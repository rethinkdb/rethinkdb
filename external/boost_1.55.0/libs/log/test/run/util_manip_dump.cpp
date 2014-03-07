/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   util_manip_dump.cpp
 * \author Andrey Semashev
 * \date   09.01.2009
 *
 * \brief  This header contains tests for the string literals wrapper.
 */

#define BOOST_TEST_MODULE util_manip_dump

#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <boost/log/utility/manipulators/dump.hpp>
#include "char_definitions.hpp"

namespace logging = boost::log;

// Test a short data region
BOOST_AUTO_TEST_CASE_TEMPLATE(unbounded_binary_lowercase_short_dump, CharT, char_types)
{
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef std::basic_ostringstream< char_type > ostream_type;

    std::vector< unsigned char > data;
    data.push_back(1);
    data.push_back(2);
    data.push_back(3);
    data.push_back(100);
    data.push_back(110);
    data.push_back(120);
    data.push_back(200);
    data.push_back(210);
    data.push_back(220);

    ostream_type strm_dump;
    strm_dump << logging::dump(&data[0], data.size());

    ostream_type strm_correct;
    strm_correct << "01 02 03 64 6e 78 c8 d2 dc";

    BOOST_CHECK(equal_strings(strm_dump.str(), strm_correct.str()));
}

// Test a short data region with uppercase formatting
BOOST_AUTO_TEST_CASE_TEMPLATE(unbounded_binary_uppercase_short_dump, CharT, char_types)
{
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef std::basic_ostringstream< char_type > ostream_type;

    std::vector< unsigned char > data;
    data.push_back(1);
    data.push_back(2);
    data.push_back(3);
    data.push_back(100);
    data.push_back(110);
    data.push_back(120);
    data.push_back(200);
    data.push_back(210);
    data.push_back(220);

    ostream_type strm_dump;
    strm_dump << std::uppercase << logging::dump(&data[0], data.size());

    ostream_type strm_correct;
    strm_correct << "01 02 03 64 6E 78 C8 D2 DC";

    BOOST_CHECK(equal_strings(strm_dump.str(), strm_correct.str()));
}

// Test void pointer handling
BOOST_AUTO_TEST_CASE_TEMPLATE(unbounded_binary_pvoid_dump, CharT, char_types)
{
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef std::basic_ostringstream< char_type > ostream_type;

    std::vector< unsigned char > data;
    data.push_back(1);
    data.push_back(2);
    data.push_back(3);
    data.push_back(100);
    data.push_back(110);
    data.push_back(120);
    data.push_back(200);
    data.push_back(210);
    data.push_back(220);

    ostream_type strm_dump;
    strm_dump << logging::dump((void*)&data[0], data.size());

    ostream_type strm_correct;
    strm_correct << "01 02 03 64 6e 78 c8 d2 dc";

    BOOST_CHECK(equal_strings(strm_dump.str(), strm_correct.str()));
}

// Test a large data region
BOOST_AUTO_TEST_CASE_TEMPLATE(unbounded_binary_large_dump, CharT, char_types)
{
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef std::basic_ostringstream< char_type > ostream_type;

    std::vector< unsigned char > data;
    ostream_type strm_correct;
    for (unsigned int i = 0; i < 1024; ++i)
    {
        unsigned char n = static_cast< unsigned char >(i);
        data.push_back(n);
        if (i > 0)
            strm_correct << " ";
        strm_correct << std::hex << std::setw(2) << std::setfill(static_cast< char_type >('0')) << static_cast< unsigned int >(n);
    }

    ostream_type strm_dump;
    strm_dump << logging::dump(&data[0], data.size());

    BOOST_CHECK(equal_strings(strm_dump.str(), strm_correct.str()));
}

// Test SIMD tail handling
BOOST_AUTO_TEST_CASE_TEMPLATE(unbounded_binary_tail_dump, CharT, char_types)
{
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef std::basic_ostringstream< char_type > ostream_type;

    std::vector< unsigned char > data;
    ostream_type strm_correct;
    // 1023 makes it very unlikely for the buffer to end at 16 or 32 byte boundary, which makes the dump algorithm to process the tail in a special way
    for (unsigned int i = 0; i < 1023; ++i)
    {
        unsigned char n = static_cast< unsigned char >(i);
        data.push_back(n);
        if (i > 0)
            strm_correct << " ";
        strm_correct << std::hex << std::setw(2) << std::setfill(static_cast< char_type >('0')) << static_cast< unsigned int >(n);
    }

    ostream_type strm_dump;
    strm_dump << logging::dump(&data[0], data.size());

    BOOST_CHECK(equal_strings(strm_dump.str(), strm_correct.str()));
}

// Test bounded dump
BOOST_AUTO_TEST_CASE_TEMPLATE(bounded_binary_dump, CharT, char_types)
{
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef std::basic_ostringstream< char_type > ostream_type;

    std::vector< unsigned char > data;
    ostream_type strm_correct;
    for (unsigned int i = 0; i < 1024; ++i)
    {
        unsigned char n = static_cast< unsigned char >(i);
        data.push_back(n);

        if (i < 500)
        {
            if (i > 0)
                strm_correct << " ";
            strm_correct << std::hex << std::setw(2) << std::setfill(static_cast< char_type >('0')) << static_cast< unsigned int >(n);
        }
    }

    strm_correct << std::dec << std::setfill(static_cast< char_type >(' ')) << " and " << 524u << " bytes more";

    ostream_type strm_dump;
    strm_dump << logging::dump(&data[0], data.size(), 500);

    BOOST_CHECK(equal_strings(strm_dump.str(), strm_correct.str()));
}

// Test array dump
BOOST_AUTO_TEST_CASE_TEMPLATE(unbounded_element_dump, CharT, char_types)
{
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef std::basic_ostringstream< char_type > ostream_type;

    std::vector< unsigned int > data;
    data.push_back(0x01020a0b);
    data.push_back(0x03040c0d);
    data.push_back(0x05060e0f);

    ostream_type strm_dump;
    strm_dump << logging::dump_elements(&data[0], data.size());

    ostream_type strm_correct;
    const unsigned char* p = reinterpret_cast< const unsigned char* >(&data[0]);
    std::size_t size = data.size() * sizeof(unsigned int);
    for (unsigned int i = 0; i < size; ++i)
    {
        unsigned char n = p[i];
        if (i > 0)
            strm_correct << " ";
        strm_correct << std::hex << std::setw(2) << std::setfill(static_cast< char_type >('0')) << static_cast< unsigned int >(n);
    }

    BOOST_CHECK(equal_strings(strm_dump.str(), strm_correct.str()));
}

// Test bounded array dump
BOOST_AUTO_TEST_CASE_TEMPLATE(bounded_element_dump, CharT, char_types)
{
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef std::basic_ostringstream< char_type > ostream_type;

    std::vector< unsigned int > data;
    data.push_back(0x01020a0b);
    data.push_back(0x03040c0d);
    data.push_back(0x05060e0f);

    ostream_type strm_dump;
    strm_dump << logging::dump_elements(&data[0], data.size(), 2);

    ostream_type strm_correct;
    const unsigned char* p = reinterpret_cast< const unsigned char* >(&data[0]);
    std::size_t size = 2 * sizeof(unsigned int);
    for (unsigned int i = 0; i < size; ++i)
    {
        unsigned char n = p[i];
        if (i > 0)
            strm_correct << " ";
        strm_correct << std::hex << std::setw(2) << std::setfill(static_cast< char_type >('0')) << static_cast< unsigned int >(n);
    }

    strm_correct << std::dec << std::setfill(static_cast< char_type >(' ')) << " and " << (data.size() - 2) * sizeof(unsigned int) << " bytes more";

    BOOST_CHECK(equal_strings(strm_dump.str(), strm_correct.str()));
}

