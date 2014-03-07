/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   filt_has_attr.cpp
 * \author Andrey Semashev
 * \date   31.01.2009
 *
 * \brief  This header contains tests for the \c has_attr filter.
 */

#define BOOST_TEST_MODULE filt_has_attr

#include <string>
#include <boost/mpl/vector.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/attribute_value_set.hpp>
#include <boost/log/expressions.hpp>
#include "char_definitions.hpp"

namespace logging = boost::log;
namespace attrs = logging::attributes;
namespace expr = logging::expressions;

// The test checks that the filter detects the attribute value presence
BOOST_AUTO_TEST_CASE(presence_check)
{
    typedef logging::attribute_set attr_set;
    typedef logging::attribute_value_set attr_values;
    typedef logging::filter filter;
    typedef test_data< char > data;

    attrs::constant< int > attr1(10);
    attrs::constant< double > attr2(5.5);
    attrs::constant< std::string > attr3("Hello, world!");

    attr_set set1, set2, set3;
    set1[data::attr1()] = attr1;
    set1[data::attr2()] = attr2;
    set1[data::attr3()] = attr3;

    attr_values values1(set1, set2, set3);
    values1.freeze();

    filter f = expr::has_attr(data::attr1());
    BOOST_CHECK(f(values1));

    f = expr::has_attr(data::attr4());
    BOOST_CHECK(!f(values1));

    f = expr::has_attr< double >(data::attr2());
    BOOST_CHECK(f(values1));

    f = expr::has_attr< double >(data::attr3());
    BOOST_CHECK(!f(values1));

    typedef mpl::vector< unsigned int, char, std::string >::type value_types;
    f = expr::has_attr< value_types >(data::attr3());
    BOOST_CHECK(f(values1));

    f = expr::has_attr< value_types >(data::attr1());
    BOOST_CHECK(!f(values1));
}

// The test checks that the filter composition works
BOOST_AUTO_TEST_CASE(composition_check)
{
    typedef logging::attribute_set attr_set;
    typedef logging::attribute_value_set attr_values;
    typedef logging::filter filter;
    typedef test_data< char > data;

    attrs::constant< int > attr1(10);
    attrs::constant< double > attr2(5.5);
    attrs::constant< std::string > attr3("Hello, world!");

    attr_set set1, set2, set3;
    attr_values values1(set1, set2, set3);
    values1.freeze();
    set1[data::attr2()] = attr2;
    attr_values values2(set1, set2, set3);
    values2.freeze();
    set1[data::attr3()] = attr3;
    set1[data::attr1()] = attr1;
    attr_values values3(set1, set2, set3);
    values3.freeze();

    filter f = expr::has_attr(data::attr1()) || expr::has_attr< double >(data::attr2());
    BOOST_CHECK(!f(values1));
    BOOST_CHECK(f(values2));
    BOOST_CHECK(f(values3));

    f = expr::has_attr(data::attr1()) && expr::has_attr< double >(data::attr2());
    BOOST_CHECK(!f(values1));
    BOOST_CHECK(!f(values2));
    BOOST_CHECK(f(values3));
}
