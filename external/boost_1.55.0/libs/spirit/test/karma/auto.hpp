//  Copyright (c) 2001-2010 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_TEST_AUTO_HPP)
#define BOOST_SPIRIT_TEST_AUTO_HPP

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/fusion/include/std_pair.hpp>
#include <boost/spirit/include/karma_bool.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/spirit/include/karma_auto.hpp>

#include "test.hpp"

namespace karma = boost::spirit::karma;
namespace traits = boost::spirit::traits;

///////////////////////////////////////////////////////////////////////////////
template <typename Char, typename T>
bool test_create_generator(Char const *expected, T const& t)
{
    std::basic_string<Char> generated;
    std::back_insert_iterator<std::basic_string<Char> > sink(generated);

    BOOST_TEST((traits::meta_create_exists<karma::domain, T>::value));
    bool result = karma::generate(sink, karma::create_generator<T>(), t);

    spirit_test::print_if_failed("test_create_generator", result, generated, expected);
    return result && generated == expected;
}

template <typename Char, typename T>
bool test_create_generator_auto(Char const *expected, T const& t)
{
    std::basic_string<Char> generated;
    std::back_insert_iterator<std::basic_string<Char> > sink(generated);

    BOOST_TEST((traits::meta_create_exists<karma::domain, T>::value));
    bool result = karma::generate(sink, t);

    spirit_test::print_if_failed("test_create_generator (auto)", result, generated, expected);
    return result && generated == expected;
}

template <typename Char, typename Attribute>
bool test_rule(Char const *expected, Attribute const& attr)
{
    BOOST_TEST((traits::meta_create_exists<karma::domain, Attribute>::value));

    typedef typename spirit_test::output_iterator<Char>::type sink_type;
    karma::rule<sink_type, Attribute()> r = 
        karma::create_generator<Attribute>();
    return spirit_test::test(expected, r, attr);
}

template <typename Char, typename Attribute, typename Delimiter>
bool test_rule_delimited(Char const *expected, Attribute const& attr
  , Delimiter const& d)
{
    BOOST_TEST((traits::meta_create_exists<karma::domain, Attribute>::value));

    typedef typename spirit_test::output_iterator<Char>::type sink_type;
    karma::rule<sink_type, Attribute(), Delimiter> r = 
        karma::create_generator<Attribute>();
    return spirit_test::test_delimited(expected, r, attr, d);
}

struct my_type {};

#endif
