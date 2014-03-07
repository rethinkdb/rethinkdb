/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   setup_settings_parser.cpp
 * \author Andrey Semashev
 * \date   25.08.2013
 *
 * \brief  This header contains tests for the settings parser.
 */

#define BOOST_TEST_MODULE setup_settings_parser

#include <string>
#include <sstream>
#include <boost/test/unit_test.hpp>
#include <boost/log/utility/setup/settings_parser.hpp>

#if !defined(BOOST_LOG_WITHOUT_SETTINGS_PARSERS)

#include <boost/log/exceptions.hpp>

namespace logging = boost::log;

typedef logging::basic_settings< char > settings;

// Tests for single-level settings
BOOST_AUTO_TEST_CASE(single_level)
{
    {
        std::istringstream strm
        (
            "[Section1]\n"
            "\n"
            "Param1 = Value1\n"
            "Param2 = \"hello, \\\"world\\\"\"\n"
            "\n"
            "[Section2]\n"
            "\n"
            "Param1 = 10\n"
            "Param2 = -2.2\n"
        );
        settings s = logging::parse_settings(strm);

        BOOST_CHECK(s.has_section("Section1"));
        BOOST_CHECK(s.has_section("Section2"));
        BOOST_CHECK(!s.has_section("Section3"));

        BOOST_CHECK(s.has_parameter("Section1", "Param1"));
        BOOST_CHECK(s.has_parameter("Section1", "Param2"));

        BOOST_CHECK(s.has_parameter("Section2", "Param1"));
        BOOST_CHECK(s.has_parameter("Section2", "Param2"));

        BOOST_CHECK_EQUAL(s["Section1"]["Param1"].or_default(std::string()), "Value1");
        BOOST_CHECK_EQUAL(s["Section1"]["Param2"].or_default(std::string()), "hello, \"world\"");

        BOOST_CHECK_EQUAL(s["Section2"]["Param1"].or_default(std::string()), "10");
        BOOST_CHECK_EQUAL(s["Section2"]["Param2"].or_default(std::string()), "-2.2");
    }
}

// Tests for multi-level settings
BOOST_AUTO_TEST_CASE(multi_level)
{
    {
        std::istringstream strm
        (
            "  [Section1]\n"
            "\n"
            "Param1 = Value1 \n"
            "Param2=\"hello, \\\"world\\\"\"   \n"
            "\n"
            "[Section1.Subsection2]  \n"
            "\n"
            "Param1=10\n"
            "Param2=-2.2\n"
        );
        settings s = logging::parse_settings(strm);

        BOOST_CHECK(s.has_section("Section1"));
        BOOST_CHECK(s.has_section("Section1.Subsection2"));
        BOOST_CHECK(!s.has_section("Subsection2"));

        BOOST_CHECK(s.has_parameter("Section1", "Param1"));
        BOOST_CHECK(s.has_parameter("Section1", "Param2"));

        BOOST_CHECK(s.has_parameter("Section1.Subsection2", "Param1"));
        BOOST_CHECK(s.has_parameter("Section1.Subsection2", "Param2"));
        BOOST_CHECK(!s.has_parameter("Subsection2", "Param1"));
        BOOST_CHECK(!s.has_parameter("Subsection2", "Param2"));

        BOOST_CHECK_EQUAL(s["Section1"]["Param1"].or_default(std::string()), "Value1");
        BOOST_CHECK_EQUAL(s["Section1"]["Param2"].or_default(std::string()), "hello, \"world\"");

        BOOST_CHECK_EQUAL(s["Section1.Subsection2"]["Param1"].or_default(std::string()), "10");
        BOOST_CHECK_EQUAL(s["Section1.Subsection2"]["Param2"].or_default(std::string()), "-2.2");
    }
}

// Tests for comments
BOOST_AUTO_TEST_CASE(comments)
{
    {
        std::istringstream strm
        (
            "# Some comment\n"
            "[ Section1 ] # another comment\n"
            "\n"
            "Param1 = Value1 ### yet another comment \n"
            "Param2=\"hello, \\\"world\\\"\" # comment after a quoted string\n"
            "\n"
            "[ Section2 ]\n"
            "\n"
            "Param1=10#comment after a number\n"
            "Param2=-2.2#comment without a terminating newline"
            "\n"
            "#[Section3]\n"
            "#\n"
            "#Param1=10#comment after a number\n"
            "#Param2=-2.2#comment without a terminating newline"
        );
        settings s = logging::parse_settings(strm);

        BOOST_CHECK(s.has_section("Section1"));
        BOOST_CHECK(s.has_section("Section2"));
        BOOST_CHECK(!s.has_section("Section3"));

        BOOST_CHECK(s.has_parameter("Section1", "Param1"));
        BOOST_CHECK(s.has_parameter("Section1", "Param2"));

        BOOST_CHECK(s.has_parameter("Section2", "Param1"));
        BOOST_CHECK(s.has_parameter("Section2", "Param2"));

        BOOST_CHECK_EQUAL(s["Section1"]["Param1"].or_default(std::string()), "Value1");
        BOOST_CHECK_EQUAL(s["Section1"]["Param2"].or_default(std::string()), "hello, \"world\"");

        BOOST_CHECK_EQUAL(s["Section2"]["Param1"].or_default(std::string()), "10");
        BOOST_CHECK_EQUAL(s["Section2"]["Param2"].or_default(std::string()), "-2.2");
    }
}

// Tests for invalid settings
BOOST_AUTO_TEST_CASE(invalid)
{
    {
        std::istringstream strm
        (
            "Param1 = Value1\n" // parameters outside sections
            "Param2 = \"hello, \\\"world\\\"\"\n"
        );
        BOOST_CHECK_THROW(logging::parse_settings(strm), logging::parse_error);
    }
    {
        std::istringstream strm
        (
            "[Section1\n" // missing closing brace
            "\n"
            "Param1 = Value1\n"
            "Param2 = \"hello, \\\"world\\\"\"\n"
            "\n"
            "[Section2]\n"
            "\n"
            "Param1 = 10\n"
            "Param2 = -2.2\n"
        );
        BOOST_CHECK_THROW(logging::parse_settings(strm), logging::parse_error);
    }
    {
        std::istringstream strm
        (
            "Section1]\n" // missing opening brace
            "\n"
            "Param1 = Value1\n"
            "Param2 = \"hello, \\\"world\\\"\"\n"
            "\n"
            "[Section2]\n"
            "\n"
            "Param1 = 10\n"
            "Param2 = -2.2\n"
        );
        BOOST_CHECK_THROW(logging::parse_settings(strm), logging::parse_error);
    }
    {
        std::istringstream strm
        (
            "[Section1=xyz]\n" // invalid characters in the section name
            "\n"
            "Param1 = Value1\n"
            "Param2 = \"hello, \\\"world\\\"\"\n"
            "\n"
            "[Section2]\n"
            "\n"
            "Param1 = 10\n"
            "Param2 = -2.2\n"
        );
        BOOST_CHECK_THROW(logging::parse_settings(strm), logging::parse_error);
    }
    {
        std::istringstream strm
        (
            "[Section1# hello?]\n" // invalid characters in the section name
            "\n"
            "Param1 = Value1\n"
            "Param2 = \"hello, \\\"world\\\"\"\n"
            "\n"
            "[Section2]\n"
            "\n"
            "Param1 = 10\n"
            "Param2 = -2.2\n"
        );
        BOOST_CHECK_THROW(logging::parse_settings(strm), logging::parse_error);
    }
    {
        std::istringstream strm
        (
            "(Section1)\n" // invalid braces
            "\n"
            "Param1 = Value1\n"
            "Param2 = \"hello, \\\"world\\\"\"\n"
            "\n"
            "[Section2]\n"
            "\n"
            "Param1 = 10\n"
            "Param2 = -2.2\n"
        );
        BOOST_CHECK_THROW(logging::parse_settings(strm), logging::parse_error);
    }
    {
        std::istringstream strm
        (
            "[Section1]\n"
            "\n"
            "Param1 =\n" // no parameter value
            "Param2 = \"hello, \\\"world\\\"\"\n"
            "\n"
            "[Section2]\n"
            "\n"
            "Param1 = 10\n"
            "Param2 = -2.2\n"
        );
        BOOST_CHECK_THROW(logging::parse_settings(strm), logging::parse_error);
    }
    {
        std::istringstream strm
        (
            "[Section1]\n"
            "\n"
            "Param1\n" // no parameter value
            "Param2 = \"hello, \\\"world\\\"\"\n"
            "\n"
            "[Section2]\n"
            "\n"
            "Param1 = 10\n"
            "Param2 = -2.2\n"
        );
        BOOST_CHECK_THROW(logging::parse_settings(strm), logging::parse_error);
    }
    {
        std::istringstream strm
        (
            "[Section1]\n"
            "\n"
            "Param1 = Value1\n"
            "Param2 = \"hello, \\\"world\\\"\n" // unterminated quote
            "\n"
            "[Section2]\n"
            "\n"
            "Param1 = 10\n"
            "Param2 = -2.2\n"
        );
        BOOST_CHECK_THROW(logging::parse_settings(strm), logging::parse_error);
    }
    {
        std::istringstream strm
        (
            "[Section1]\n"
            "\n"
            "Param1 = Value1 Value2\n" // multi-word value
            "Param2 = \"hello, \\\"world\\\"\"\n"
            "\n"
            "[Section2]\n"
            "\n"
            "Param1 = 10\n"
            "Param2 = -2.2\n"
        );
        BOOST_CHECK_THROW(logging::parse_settings(strm), logging::parse_error);
    }
}

#endif // !defined(BOOST_LOG_WITHOUT_SETTINGS_PARSERS)
