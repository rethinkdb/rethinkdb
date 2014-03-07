/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_STANDARD_APRIL_26_2006_1106PM)
#define BOOST_SPIRIT_STANDARD_APRIL_26_2006_1106PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <cctype>
#include <boost/cstdint.hpp>

namespace boost { namespace spirit { namespace char_encoding
{
    ///////////////////////////////////////////////////////////////////////////
    //  Test characters for specified conditions (using std functions)
    ///////////////////////////////////////////////////////////////////////////
    struct standard
    {
        typedef char char_type;

        static bool
        isascii_(int ch)
        {
            return 0 == (ch & ~0x7f);
        }

        static bool
        ischar(int ch)
        {
            // uses all 8 bits
            // we have to watch out for sign extensions
            return (0 == (ch & ~0xff) || ~0 == (ch | 0xff)) ? true : false;
        }

        static int
        isalnum(int ch)
        {
            return std::isalnum(ch);
        }

        static int
        isalpha(int ch)
        {
            return std::isalpha(ch);
        }

        static int
        isdigit(int ch)
        {
            return std::isdigit(ch);
        }

        static int
        isxdigit(int ch)
        {
            return std::isxdigit(ch);
        }

        static int
        iscntrl(int ch)
        {
            return std::iscntrl(ch);
        }

        static int
        isgraph(int ch)
        {
            return std::isgraph(ch);
        }

        static int
        islower(int ch)
        {
            return std::islower(ch);
        }

        static int
        isprint(int ch)
        {
            return std::isprint(ch);
        }

        static int
        ispunct(int ch)
        {
            return std::ispunct(ch);
        }

        static int
        isspace(int ch)
        {
            return std::isspace(ch);
        }

        static int
        isblank BOOST_PREVENT_MACRO_SUBSTITUTION (int ch)
        {
            return (ch == ' ' || ch == '\t');
        }

        static int
        isupper(int ch)
        {
            return std::isupper(ch);
        }

    ///////////////////////////////////////////////////////////////////////////////
    //  Simple character conversions
    ///////////////////////////////////////////////////////////////////////////////
        static int
        tolower(int ch)
        {
            return std::tolower(ch);
        }

        static int
        toupper(int ch)
        {
            return std::toupper(ch);
        }

        static ::boost::uint32_t
        toucs4(int ch)
        {
            return ch;
        }
    };
}}}

#endif

