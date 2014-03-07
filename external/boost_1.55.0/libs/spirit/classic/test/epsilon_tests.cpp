/*=============================================================================
    Copyright (c) 2002-2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <iostream>
#include <cstring>
#include <boost/detail/lightweight_test.hpp>

// This test program only includes the epsilon.hpp header from Spirit
#include <boost/spirit/include/classic_epsilon.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "impl/var.hpp"
#include "impl/string_length.hpp"

using namespace test;
static BOOST_SPIRIT_CLASSIC_NS::parse_info<char const *> pi;

////////////////////////////////////////////////
// These macros are used with BOOST_TEST
#define matches (pi.hit)
#define full_match (pi.hit && pi.full)
#define partial_match (pi.hit && !pi.full)
#define no_match (!pi.hit && !pi.full)
#define zero_length_match (pi.length == 0)
#define stop_equals_start (pi.stop == s)

template<typename ParserT>
static void
parse(char const *s, ParserT const &p, bool match)
{
    
    pi = BOOST_SPIRIT_CLASSIC_NS::parse(s, s + test_impl::string_length(s), p);
    if (match)
    {
        BOOST_TEST(matches);
        BOOST_TEST(zero_length_match);
        BOOST_TEST(stop_equals_start);
    }
    else
    {
        BOOST_TEST(no_match);
    }
}

static char const empty[] = "";
static char const not_empty[] = "asdfgh";

////////////////////////////////////////////////
// Test wether epsilon_p/eps_p work as
// primitive parsers
static void
epsilon_as_primitive()
{
    // This test case also is a compile time check wether
    // both eps_p and epsilon_p are present.

    parse(empty, BOOST_SPIRIT_CLASSIC_NS::epsilon_p, true);
    BOOST_TEST(full_match);
    parse(not_empty, BOOST_SPIRIT_CLASSIC_NS::epsilon_p, true);
    BOOST_TEST(partial_match);

    parse(empty, BOOST_SPIRIT_CLASSIC_NS::eps_p, true);
    BOOST_TEST(full_match);
    parse(not_empty, BOOST_SPIRIT_CLASSIC_NS::eps_p, true);
    BOOST_TEST(partial_match);
}

////////////////////////////////////////////////
// Test wether epsilon_p/eps_p work correctly as
// a parser generator for creating parsers from
// functors
static void
epsilon_as_parser_generator_for_functors()
{
    bool       flag = false;
    parse(empty, BOOST_SPIRIT_CLASSIC_NS::epsilon_p(var(flag)), flag);
    BOOST_TEST(no_match);

    flag = true;
    parse(empty, BOOST_SPIRIT_CLASSIC_NS::epsilon_p(var(flag)), flag);
    BOOST_TEST(full_match);
}

////////////////////////////////////////////////
// Test wether epsilon_p/eps_p work correctly as
// a parser generator for creating parsers from
// other parsers
static void
epsilon_as_parser_generator_for_parsers()
{
    // This test case uses a parser created by epsilon_p
    // as body-parser for another invokation of epsilon_p

    bool        flag = false;
    parse(empty, BOOST_SPIRIT_CLASSIC_NS::epsilon_p(
            BOOST_SPIRIT_CLASSIC_NS::epsilon_p(var(flag))), flag);
    BOOST_TEST(no_match);

    flag = true;
    parse(empty, BOOST_SPIRIT_CLASSIC_NS::epsilon_p(
            BOOST_SPIRIT_CLASSIC_NS::epsilon_p(var(flag))), flag);
    BOOST_TEST(full_match);
}

////////////////////////////////////////////////
// Test wether epsilon_p/eps_p support negation
static void
negation_operator_for_epsilon()
{
    bool       flag = false;
    parse(empty, ~BOOST_SPIRIT_CLASSIC_NS::epsilon_p(var(flag)), !flag);
    BOOST_TEST(full_match);
    parse(empty, ~~BOOST_SPIRIT_CLASSIC_NS::epsilon_p(var(flag)), flag);
    BOOST_TEST(no_match);

    flag = true;
    parse(empty, ~BOOST_SPIRIT_CLASSIC_NS::epsilon_p(var(flag)), !flag);
    BOOST_TEST(no_match);
    parse(empty, ~~BOOST_SPIRIT_CLASSIC_NS::epsilon_p(var(flag)), flag);
    BOOST_TEST(full_match);
}

int
main()
{
    epsilon_as_primitive();
    epsilon_as_parser_generator_for_functors();
    epsilon_as_parser_generator_for_parsers();
    negation_operator_for_epsilon();

    return boost::report_errors();
}
