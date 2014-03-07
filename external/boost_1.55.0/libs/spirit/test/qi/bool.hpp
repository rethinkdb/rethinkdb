/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2011      Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_TEST_QI_BOOL)
#define BOOST_SPIRIT_TEST_QI_BOOL

#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/support_argument.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_directive.hpp>
#include <boost/cstdint.hpp>
#include "test.hpp"

///////////////////////////////////////////////////////////////////////////////
struct backwards_bool_policies : boost::spirit::qi::bool_policies<>
{
    // we want to interpret a 'true' spelled backwards as 'false'
    template <typename Iterator, typename Attribute>
    static bool
    parse_false(Iterator& first, Iterator const& last, Attribute& attr)
    {
        namespace spirit = boost::spirit;
        namespace qi = boost::spirit::qi;
        if (qi::detail::string_parse("eurt", first, last, qi::unused))
        {
            spirit::traits::assign_to(false, attr);    // result is false
            return true;
        }
        return false;
    }
};

///////////////////////////////////////////////////////////////////////////////
struct test_bool_type
{
    test_bool_type(bool b) : b(b) {}    // provide conversion
    bool b;
};

#endif

