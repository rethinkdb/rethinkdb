/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   util_string_literal.cpp
 * \author Andrey Semashev
 * \date   09.01.2009
 *
 * \brief  This header contains tests for the string literals wrapper.
 */

#define BOOST_TEST_MODULE util_string_literal

#include <cwchar>
#include <set>
#include <memory>
#include <string>
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <boost/log/utility/string_literal.hpp>

namespace logging = boost::log;

// Construction tests
BOOST_AUTO_TEST_CASE(string_literal_ctors)
{
    // Default construction
    {
        logging::string_literal lit;
        BOOST_CHECK(lit.empty());
        BOOST_CHECK_EQUAL(lit.size(), 0UL);
        BOOST_CHECK(lit.c_str() != NULL);
    }

    // Construction from a literal
    {
        logging::string_literal lit = "abcd";
        BOOST_CHECK(!lit.empty());
        BOOST_CHECK_EQUAL(lit.size(), 4UL);
        BOOST_CHECK(strcmp(lit.c_str(), "abcd") == 0);
    }

    // Copying
    {
        logging::wstring_literal lit1 = L"Hello";
        logging::wstring_literal lit2 = lit1;
        BOOST_CHECK(wcscmp(lit2.c_str(), L"Hello") == 0);
        BOOST_CHECK(wcscmp(lit1.c_str(), lit2.c_str()) == 0);
    }

    // Generator functions
    {
        logging::string_literal lit1 = logging::str_literal("Wow!");
        BOOST_CHECK(strcmp(lit1.c_str(), "Wow!") == 0);

        logging::wstring_literal lit2 = logging::str_literal(L"Wow!");
        BOOST_CHECK(wcscmp(lit2.c_str(), L"Wow!") == 0);
    }
}

// Assignment tests
BOOST_AUTO_TEST_CASE(string_literal_assignment)
{
    // operator=
    {
        logging::string_literal lit;
        BOOST_CHECK(lit.empty());

        lit = "Hello";
        BOOST_CHECK(strcmp(lit.c_str(), "Hello") == 0);

        logging::string_literal empty_lit;
        lit = empty_lit;
        BOOST_CHECK(lit.empty());

        logging::string_literal filled_lit = "Some string";
        lit = filled_lit;
        BOOST_CHECK(strcmp(lit.c_str(), filled_lit.c_str()) == 0);
    }

    // assign
    {
        logging::string_literal lit;
        BOOST_CHECK(lit.empty());

        lit.assign("Hello");
        BOOST_CHECK(strcmp(lit.c_str(), "Hello") == 0);

        logging::string_literal empty_lit;
        lit.assign(empty_lit);
        BOOST_CHECK(lit.empty());

        logging::string_literal filled_lit = "Some string";
        lit.assign(filled_lit);
        BOOST_CHECK(strcmp(lit.c_str(), filled_lit.c_str()) == 0);
    }
}

// Comparison tests
BOOST_AUTO_TEST_CASE(string_literal_comparison)
{
    logging::string_literal lit;
    BOOST_CHECK(lit == "");

    lit = "abcdefg";
    BOOST_CHECK(lit == "abcdefg");
    BOOST_CHECK(lit != "xyz");
    BOOST_CHECK(lit != "aBcDeFg");

    logging::string_literal lit2 = "Yo!";
    BOOST_CHECK(lit != lit2);
    lit2 = "abcdefg";
    BOOST_CHECK(lit == lit2);

    BOOST_CHECK(lit.compare(lit2) == 0);
    BOOST_CHECK(lit.compare("aaaaa") > 0);
    BOOST_CHECK(lit.compare("zzzzzzzz") < 0);
    BOOST_CHECK(lit.compare("zbcdefg") < 0);

    BOOST_CHECK(lit.compare(2, 3, "cde") == 0);
    BOOST_CHECK(lit.compare(2, 3, "cdefgh", 3) == 0);

    // Check ordering
    std::set< logging::string_literal > lit_set;
    lit_set.insert(logging::str_literal("abc"));
    lit_set.insert(logging::str_literal("def"));
    lit_set.insert(logging::str_literal("aaa"));
    lit_set.insert(logging::str_literal("abcd"));
    lit_set.insert(logging::str_literal("zz"));

    std::set< std::string > str_set;
    str_set.insert(logging::str_literal("abc").str());
    str_set.insert(logging::str_literal("def").str());
    str_set.insert(logging::str_literal("aaa").str());
    str_set.insert(logging::str_literal("abcd").str());
    str_set.insert(logging::str_literal("zz").str());

    BOOST_CHECK_EQUAL_COLLECTIONS(lit_set.begin(), lit_set.end(), str_set.begin(), str_set.end());
}

// Iteration tests
BOOST_AUTO_TEST_CASE(string_literal_iteration)
{
    std::string str;
    logging::string_literal lit = "abcdefg";

    std::copy(lit.begin(), lit.end(), std::back_inserter(str));
    BOOST_CHECK(str == "abcdefg");

    str.clear();
    std::copy(lit.rbegin(), lit.rend(), std::back_inserter(str));
    BOOST_CHECK(str == "gfedcba");
}

// Subscript tests
BOOST_AUTO_TEST_CASE(string_literal_indexing)
{
    logging::string_literal lit = "abcdefg";

    BOOST_CHECK_EQUAL(lit[2], 'c');
    BOOST_CHECK_EQUAL(lit.at(3), 'd');

    BOOST_CHECK_THROW(lit.at(100), std::exception);
}

// Clearing tests
BOOST_AUTO_TEST_CASE(string_literal_clear)
{
    logging::string_literal lit = "yo-ho-ho";
    BOOST_CHECK(!lit.empty());

    lit.clear();
    BOOST_CHECK(lit.empty());
    BOOST_CHECK_EQUAL(lit, "");
}

// Swapping tests
BOOST_AUTO_TEST_CASE(string_literal_swap)
{
    logging::string_literal lit1 = "yo-ho-ho";
    logging::string_literal lit2 = "hello";

    lit1.swap(lit2);
    BOOST_CHECK_EQUAL(lit1, "hello");
    BOOST_CHECK_EQUAL(lit2, "yo-ho-ho");

    swap(lit1, lit2);
    BOOST_CHECK_EQUAL(lit2, "hello");
    BOOST_CHECK_EQUAL(lit1, "yo-ho-ho");
}

// STL strings acquisition tests
BOOST_AUTO_TEST_CASE(string_literal_str)
{
    logging::string_literal lit = "yo-ho-ho";
    std::string str = lit.str();
    BOOST_CHECK_EQUAL(str, "yo-ho-ho");
    BOOST_CHECK_EQUAL(lit, str);
}

// Substring extraction tests
BOOST_AUTO_TEST_CASE(string_literal_copy)
{
    logging::string_literal lit = "yo-ho-ho";
    char t[32] = { 0 };

    BOOST_CHECK_EQUAL(lit.copy(t, 2), 2UL);
    BOOST_CHECK(strcmp(t, "yo") == 0);

    BOOST_CHECK_EQUAL(lit.copy(t, 4, 2), 4UL);
    BOOST_CHECK(strcmp(t, "-ho-") == 0);

    BOOST_CHECK_EQUAL(lit.copy(t, 100), 8UL);
    BOOST_CHECK(strcmp(t, "yo-ho-ho") == 0);
}
