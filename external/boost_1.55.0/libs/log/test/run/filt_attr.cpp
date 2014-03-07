/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   filt_attr.cpp
 * \author Andrey Semashev
 * \date   31.01.2009
 *
 * \brief  This header contains tests for the \c attr filter.
 */

#define BOOST_TEST_MODULE filt_attr

#include <memory>
#include <string>
#include <algorithm>
#include <boost/regex.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/phoenix/bind.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/attribute_value_set.hpp>
#include <boost/log/utility/type_dispatch/standard_types.hpp>
#include <boost/log/support/regex.hpp>
#include <boost/log/expressions.hpp>
#include "char_definitions.hpp"

namespace phoenix = boost::phoenix;

namespace logging = boost::log;
namespace attrs = logging::attributes;
namespace expr = logging::expressions;

// The test checks that general conditions work
BOOST_AUTO_TEST_CASE(general_conditions)
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

    filter f = expr::attr< int >(data::attr1()) == 10;
    BOOST_CHECK(f(values1));

    f = expr::attr< int >(data::attr1()) < 0;
    BOOST_CHECK(!f(values1));

    f = expr::attr< float >(data::attr1()).or_throw() > 0;
    BOOST_CHECK_THROW(f(values1), logging::runtime_error);
    f = expr::attr< float >(data::attr1()) > 0;
    BOOST_CHECK(!f(values1));

    f = expr::attr< int >(data::attr4()).or_throw() >= 1;
    BOOST_CHECK_THROW(f(values1), logging::runtime_error);
    f = expr::attr< int >(data::attr4()) >= 1;
    BOOST_CHECK(!f(values1));

    f = expr::attr< int >(data::attr4()) < 1;
    BOOST_CHECK(!f(values1));

    f = expr::attr< logging::numeric_types >(data::attr2()) > 5;
    BOOST_CHECK(f(values1));

    f = expr::attr< std::string >(data::attr3()) == "Hello, world!";
    BOOST_CHECK(f(values1));

    f = expr::attr< std::string >(data::attr3()) > "AAA";
    BOOST_CHECK(f(values1));
}

// The test checks that is_in_range condition works
BOOST_AUTO_TEST_CASE(in_range_check)
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

    filter f = expr::is_in_range(expr::attr< int >(data::attr1()), 5, 20);
    BOOST_CHECK(f(values1));

    f = expr::is_in_range(expr::attr< int >(data::attr1()), 5, 10);
    BOOST_CHECK(!f(values1));

    f = expr::is_in_range(expr::attr< int >(data::attr1()), 10, 20);
    BOOST_CHECK(f(values1));

    f = expr::is_in_range(expr::attr< logging::numeric_types >(data::attr2()), 5, 6);
    BOOST_CHECK(f(values1));

    f = expr::is_in_range(expr::attr< std::string >(data::attr3()), "AAA", "zzz");
    BOOST_CHECK(f(values1));

    // Check that strings are saved into the filter by value
    char buf1[128];
    char buf2[128];
    std::strcpy(buf1, "AAA");
    std::strcpy(buf2, "zzz");
    f = expr::is_in_range(expr::attr< std::string >(data::attr3()), buf1, buf2);
    std::fill_n(buf1, sizeof(buf1), static_cast< char >(0));
    std::fill_n(buf2, sizeof(buf2), static_cast< char >(0));
    BOOST_CHECK(f(values1));

    std::strcpy(buf1, "AAA");
    std::strcpy(buf2, "zzz");
    f = expr::is_in_range(expr::attr< std::string >(data::attr3()),
        static_cast< const char* >(buf1), static_cast< const char* >(buf2));
    std::fill_n(buf1, sizeof(buf1), static_cast< char >(0));
    std::fill_n(buf2, sizeof(buf2), static_cast< char >(0));
    BOOST_CHECK(f(values1));
}

namespace {

    struct predicate
    {
        typedef bool result_type;

        explicit predicate(unsigned int& present_counter, bool& result) :
            m_PresentCounter(present_counter),
            m_Result(result)
        {
        }

        template< typename T, typename TagT >
        result_type operator() (logging::value_ref< T, TagT > const& val) const
        {
            m_PresentCounter += !val.empty();
            return m_Result;
        }

    private:
        unsigned int& m_PresentCounter;
        bool& m_Result;
    };

} // namespace

// The test checks that phoenix::bind interaction works
BOOST_AUTO_TEST_CASE(bind_support_check)
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

    unsigned int present_counter = 0;
    bool predicate_result = false;

    filter f = phoenix::bind(predicate(present_counter, predicate_result), expr::attr< int >(data::attr1()));
    BOOST_CHECK_EQUAL(f(values1), predicate_result);
    BOOST_CHECK_EQUAL(present_counter, 1U);

    predicate_result = true;
    BOOST_CHECK_EQUAL(f(values1), predicate_result);
    BOOST_CHECK_EQUAL(present_counter, 2U);

    f = phoenix::bind(predicate(present_counter, predicate_result), expr::attr< logging::numeric_types >(data::attr2()));
    BOOST_CHECK_EQUAL(f(values1), predicate_result);
    BOOST_CHECK_EQUAL(present_counter, 3U);

    f = phoenix::bind(predicate(present_counter, predicate_result), expr::attr< int >(data::attr2()).or_throw());
    BOOST_CHECK_THROW(f(values1), logging::runtime_error);
    f = phoenix::bind(predicate(present_counter, predicate_result), expr::attr< int >(data::attr2()));
    BOOST_CHECK_EQUAL(f(values1), true);
    BOOST_CHECK_EQUAL(present_counter, 3U);

    f = phoenix::bind(predicate(present_counter, predicate_result), expr::attr< int >(data::attr4()).or_throw());
    BOOST_CHECK_THROW(f(values1), logging::runtime_error);
    f = phoenix::bind(predicate(present_counter, predicate_result), expr::attr< int >(data::attr4()));
    BOOST_CHECK_EQUAL(f(values1), true);
    BOOST_CHECK_EQUAL(present_counter, 3U);
}

// The test checks that begins_with condition works
BOOST_AUTO_TEST_CASE(begins_with_check)
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

    filter f = expr::begins_with(expr::attr< std::string >(data::attr3()), "Hello");
    BOOST_CHECK(f(values1));

    f = expr::begins_with(expr::attr< std::string >(data::attr3()), "hello");
    BOOST_CHECK(!f(values1));

    f = expr::begins_with(expr::attr< std::string >(data::attr3()).or_throw(), "Bye");
    BOOST_CHECK(!f(values1));

    f = expr::begins_with(expr::attr< std::string >(data::attr3()).or_throw(), "world!");
    BOOST_CHECK(!f(values1));

    f = expr::begins_with(expr::attr< std::string >(data::attr2()), "Hello");
    BOOST_CHECK(!f(values1));

    f = expr::begins_with(expr::attr< std::string >(data::attr4()), "Hello");
    BOOST_CHECK(!f(values1));
}

// The test checks that ends_with condition works
BOOST_AUTO_TEST_CASE(ends_with_check)
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

    filter f = expr::ends_with(expr::attr< std::string >(data::attr3()), "world!");
    BOOST_CHECK(f(values1));

    f = expr::ends_with(expr::attr< std::string >(data::attr3()), "World!");
    BOOST_CHECK(!f(values1));

    f = expr::ends_with(expr::attr< std::string >(data::attr3()).or_throw(), "Bye");
    BOOST_CHECK(!f(values1));

    f = expr::ends_with(expr::attr< std::string >(data::attr3()).or_throw(), "Hello");
    BOOST_CHECK(!f(values1));

    f = expr::ends_with(expr::attr< std::string >(data::attr2()), "world!");
    BOOST_CHECK(!f(values1));

    f = expr::ends_with(expr::attr< std::string >(data::attr4()), "world!");
    BOOST_CHECK(!f(values1));
}

// The test checks that contains condition works
BOOST_AUTO_TEST_CASE(contains_check)
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

    filter f = expr::contains(expr::attr< std::string >(data::attr3()), "Hello");
    BOOST_CHECK(f(values1));

    f = expr::contains(expr::attr< std::string >(data::attr3()), "hello");
    BOOST_CHECK(!f(values1));

    f = expr::contains(expr::attr< std::string >(data::attr3()).or_throw(), "o, w");
    BOOST_CHECK(f(values1));

    f = expr::contains(expr::attr< std::string >(data::attr3()).or_throw(), "world!");
    BOOST_CHECK(f(values1));

    f = expr::contains(expr::attr< std::string >(data::attr2()), "Hello");
    BOOST_CHECK(!f(values1));

    f = expr::contains(expr::attr< std::string >(data::attr4()), "Hello");
    BOOST_CHECK(!f(values1));
}

// The test checks that matches condition works
BOOST_AUTO_TEST_CASE(matches_check)
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

    boost::regex rex("hello");
    filter f = expr::matches(expr::attr< std::string >(data::attr3()), rex);
    BOOST_CHECK(!f(values1));

    rex = ".*world.*";
    f = expr::matches(expr::attr< std::string >(data::attr3()).or_throw(), rex);
    BOOST_CHECK(f(values1));

    rex = ".*";
    f = expr::matches(expr::attr< std::string >(data::attr2()), rex);
    BOOST_CHECK(!f(values1));

    f = expr::matches(expr::attr< std::string >(data::attr4()), rex);
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

    filter f =
        expr::attr< int >(data::attr1()) <= 10 ||
        expr::is_in_range(expr::attr< double >(data::attr2()), 2.2, 7.7);
    BOOST_CHECK(!f(values1));
    BOOST_CHECK(f(values2));
    BOOST_CHECK(f(values3));

    f = expr::attr< int >(data::attr1()) == 10 &&
        expr::begins_with(expr::attr< std::string >(data::attr3()), "Hello");
    BOOST_CHECK(!f(values1));
    BOOST_CHECK(!f(values2));
    BOOST_CHECK(f(values3));
}
