/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   char_definitions.hpp
 * \author Andrey Semashev
 * \date   24.01.2009
 *
 * \brief  This header contains common type definitions for character type dependent tests.
 */

#ifndef BOOST_LOG_TESTS_CHAR_DEFINITIONS_HPP_INCLUDED_
#define BOOST_LOG_TESTS_CHAR_DEFINITIONS_HPP_INCLUDED_

#include <string>
#include <iostream>
#include <boost/mpl/vector.hpp>

namespace mpl = boost::mpl;

typedef mpl::vector<
#ifdef BOOST_LOG_USE_CHAR
    char
#endif
#if defined(BOOST_LOG_USE_CHAR) && defined(BOOST_LOG_USE_WCHAR_T)
    ,
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
    wchar_t
#endif
>::type char_types;

template< typename >
struct test_data;


#ifdef BOOST_LOG_USE_CHAR

template< >
struct test_data< char >
{
    static const char* abc() { return "abc"; }
    static const char* ABC() { return "ABC"; }
    static const char* some_test_string() { return "some test string"; }
    static const char* zero_to_five() { return "012345"; }
    static const char* def() { return "def"; }
    static const char* aaa() { return "aaa"; }
    static const char* abcd() { return "abcd"; }
    static const char* zz() { return "zz"; }
    static const char* abcdefg0123456789() { return "abcdefg0123456789"; }

    static const char* attr1() { return "attr1"; }
    static const char* attr2() { return "attr2"; }
    static const char* attr3() { return "attr3"; }
    static const char* attr4() { return "attr4"; }

    static const char* int_format1() { return "%08d"; }
    static const char* fp_format1() { return "%06.3f"; }
};

//! The function compares two strings and prints them if they are not equal
inline bool equal_strings(std::string const& left, std::string const& right)
{
    if (left != right)
    {
        std::cout << "Left:  \"" << left << "\"\nRight: \"" << right << "\"" << std::endl;
        return false;
    }
    else
        return true;
}

#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

template< >
struct test_data< wchar_t >
{
    static const wchar_t* abc() { return L"abc"; }
    static const wchar_t* ABC() { return L"ABC"; }
    static const wchar_t* some_test_string() { return L"some test string"; }
    static const wchar_t* zero_to_five() { return L"012345"; }
    static const wchar_t* def() { return L"def"; }
    static const wchar_t* aaa() { return L"aaa"; }
    static const wchar_t* abcd() { return L"abcd"; }
    static const wchar_t* zz() { return L"zz"; }
    static const wchar_t* abcdefg0123456789() { return L"abcdefg0123456789"; }

    static const char* attr1() { return "attr1"; }
    static const char* attr2() { return "attr2"; }
    static const char* attr3() { return "attr3"; }
    static const char* attr4() { return "attr4"; }

    static const wchar_t* int_format1() { return L"%08d"; }
    static const wchar_t* fp_format1() { return L"%06.3f"; }
};

//! The function compares two strings and prints them if they are not equal
inline bool equal_strings(std::wstring const& left, std::wstring const& right)
{
    if (left != right)
    {
        std::wcout << L"Left:  \"" << left << L"\"\nRight: \"" << right << L"\"" << std::endl;
        return false;
    }
    else
        return true;
}

#endif // BOOST_LOG_USE_WCHAR_T

#endif // BOOST_LOG_TESTS_CHAR_DEFINITIONS_HPP_INCLUDED_
