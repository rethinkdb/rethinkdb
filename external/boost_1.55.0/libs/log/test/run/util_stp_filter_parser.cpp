/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   setup_filter_parser.cpp
 * \author Andrey Semashev
 * \date   24.08.2013
 *
 * \brief  This header contains tests for the filter parser.
 */

#define BOOST_TEST_MODULE setup_filter_parser

#include <string>
#include <boost/test/unit_test.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>

#if !defined(BOOST_LOG_WITHOUT_SETTINGS_PARSERS) && !defined(BOOST_LOG_WITHOUT_DEFAULT_FACTORIES)

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/attribute_value_set.hpp>
#include <boost/log/expressions/filter.hpp>

namespace logging = boost::log;
namespace attrs = logging::attributes;

typedef logging::attribute_set attr_set;
typedef logging::attribute_value_set attr_values;

// Tests for attribute presence check
BOOST_AUTO_TEST_CASE(attr_presence)
{
    attrs::constant< int > attr1(10);
    attr_set set1, set2, set3;

    attr_values values1(set1, set2, set3);
    values1.freeze();

    set1["MyAttr"] = attr1;
    attr_values values2(set1, set2, set3);
    values2.freeze();

    {
        logging::filter f = logging::parse_filter("%MyAttr%");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
    }
    {
        logging::filter f = logging::parse_filter(" % MyAttr % ");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
    }
}

// Tests for integer relation filter
BOOST_AUTO_TEST_CASE(int_relation)
{
    attrs::constant< int > attr1(10);
    attrs::constant< long > attr2(20);
    attrs::constant< int > attr3(-2);
    attr_set set1, set2, set3;

    attr_values values1(set1, set2, set3);
    values1.freeze();

    set1["MyAttr"] = attr1;
    attr_values values2(set1, set2, set3);
    values2.freeze();

    set1["MyAttr"] = attr2;
    attr_values values3(set1, set2, set3);
    values3.freeze();

    set1["MyAttr"] = attr3;
    attr_values values4(set1, set2, set3);
    values4.freeze();

    {
        logging::filter f = logging::parse_filter("%MyAttr% = 10");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
        BOOST_CHECK(!f(values3));
        BOOST_CHECK(!f(values4));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% != 10");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
        BOOST_CHECK(f(values3));
        BOOST_CHECK(f(values4));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% < 20");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
        BOOST_CHECK(!f(values3));
        BOOST_CHECK(f(values4));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% < -7");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
        BOOST_CHECK(!f(values3));
        BOOST_CHECK(!f(values4));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% > 10");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
        BOOST_CHECK(f(values3));
        BOOST_CHECK(!f(values4));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% > -5");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
        BOOST_CHECK(f(values3));
        BOOST_CHECK(f(values4));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% <= 20");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
        BOOST_CHECK(f(values3));
        BOOST_CHECK(f(values4));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% >= 20");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
        BOOST_CHECK(f(values3));
        BOOST_CHECK(!f(values4));
    }
}

// Tests for floating point relation filter
BOOST_AUTO_TEST_CASE(fp_relation)
{
    attrs::constant< float > attr1(2.5);
    attrs::constant< float > attr2(8.8);
    attrs::constant< double > attr3(-9.1);
    attrs::constant< float > attr4(0);
    attr_set set1, set2, set3;

    attr_values values1(set1, set2, set3);
    values1.freeze();

    set1["MyAttr"] = attr1;
    attr_values values2(set1, set2, set3);
    values2.freeze();

    set1["MyAttr"] = attr2;
    attr_values values3(set1, set2, set3);
    values3.freeze();

    set1["MyAttr"] = attr3;
    attr_values values4(set1, set2, set3);
    values4.freeze();

    set1["MyAttr"] = attr4;
    attr_values values5(set1, set2, set3);
    values5.freeze();

    {
        logging::filter f = logging::parse_filter("%MyAttr% = 10.3");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
        BOOST_CHECK(!f(values3));
        BOOST_CHECK(!f(values4));
        BOOST_CHECK(!f(values5));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% != 10");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
        BOOST_CHECK(f(values3));
        BOOST_CHECK(f(values4));
        BOOST_CHECK(f(values5));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% < 5.5");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
        BOOST_CHECK(!f(values3));
        BOOST_CHECK(f(values4));
        BOOST_CHECK(f(values5));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% < -7");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
        BOOST_CHECK(!f(values3));
        BOOST_CHECK(f(values4));
        BOOST_CHECK(!f(values5));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% > 5.6");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
        BOOST_CHECK(f(values3));
        BOOST_CHECK(!f(values4));
        BOOST_CHECK(!f(values5));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% > -5");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
        BOOST_CHECK(f(values3));
        BOOST_CHECK(!f(values4));
        BOOST_CHECK(f(values5));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% <= 20");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
        BOOST_CHECK(f(values3));
        BOOST_CHECK(f(values4));
        BOOST_CHECK(f(values5));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% >= 20");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
        BOOST_CHECK(!f(values3));
        BOOST_CHECK(!f(values4));
        BOOST_CHECK(!f(values5));
    }
}

// Tests for string relation filter
BOOST_AUTO_TEST_CASE(string_relation)
{
    attrs::constant< std::string > attr1("hello");
    attr_set set1, set2, set3;

    attr_values values1(set1, set2, set3);
    values1.freeze();

    set1["MyStr"] = attr1;
    attr_values values2(set1, set2, set3);
    values2.freeze();

    {
        logging::filter f = logging::parse_filter("%MyStr% = hello");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% = \"hello\"");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
    }
    {
        logging::filter f = logging::parse_filter(" % MyStr % = \"hello\" ");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% = \" hello\"");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% = \"hello \"");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% = \"world\"");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% = \"Hello\"");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% != hello");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% != world");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% < world");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% > world");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
    }

    // Check that strings that look like numbers can still be used in filters
    attrs::constant< std::string > attr2("55");
    set1["MyStr"] = attr2;
    attr_values values3(set1, set2, set3);
    values3.freeze();

    {
        logging::filter f = logging::parse_filter("%MyStr% = \"55\"");
        BOOST_CHECK(f(values3));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% < \"555\"");
        BOOST_CHECK(f(values3));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% > \"44\"");
        BOOST_CHECK(f(values3));
    }
}

// Tests for multiple expression filter
BOOST_AUTO_TEST_CASE(multi_expression)
{
    attrs::constant< int > attr1(10);
    attrs::constant< int > attr2(20);
    attrs::constant< std::string > attr3("hello");
    attrs::constant< std::string > attr4("world");
    attr_set set1, set2, set3;

    attr_values values1(set1, set2, set3);
    values1.freeze();

    set1["MyAttr"] = attr1;
    attr_values values2(set1, set2, set3);
    values2.freeze();

    set1["MyAttr"] = attr2;
    attr_values values3(set1, set2, set3);
    values3.freeze();

    set1["MyStr"] = attr3;
    attr_values values4(set1, set2, set3);
    values4.freeze();

    set1["MyStr"] = attr4;
    attr_values values5(set1, set2, set3);
    values5.freeze();

    {
        logging::filter f = logging::parse_filter("%MyAttr% = 10 & %MyStr% = \"hello\"");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
        BOOST_CHECK(!f(values3));
        BOOST_CHECK(!f(values4));
        BOOST_CHECK(!f(values5));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% > 10 & %MyStr% = \"hello\"");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
        BOOST_CHECK(!f(values3));
        BOOST_CHECK(f(values4));
        BOOST_CHECK(!f(values5));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% > 10 and %MyStr% = \"hello\"");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
        BOOST_CHECK(!f(values3));
        BOOST_CHECK(f(values4));
        BOOST_CHECK(!f(values5));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% = 10 | %MyStr% = \"world\"");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
        BOOST_CHECK(!f(values3));
        BOOST_CHECK(!f(values4));
        BOOST_CHECK(f(values5));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% > 10 | %MyStr% = \"world\"");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(!f(values2));
        BOOST_CHECK(f(values3));
        BOOST_CHECK(f(values4));
        BOOST_CHECK(f(values5));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% = 10 or %MyStr% = \"world\"");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
        BOOST_CHECK(!f(values3));
        BOOST_CHECK(!f(values4));
        BOOST_CHECK(f(values5));
    }
    {
        logging::filter f = logging::parse_filter("%MyAttr% > 0 & %MyAttr% < 20 | %MyStr% = \"hello\"");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
        BOOST_CHECK(!f(values3));
        BOOST_CHECK(f(values4));
        BOOST_CHECK(!f(values5));
    }
}

// Tests for negation
BOOST_AUTO_TEST_CASE(negation)
{
    attrs::constant< int > attr1(10);
    attrs::constant< int > attr2(20);
    attrs::constant< std::string > attr3("hello");
    attr_set set1, set2, set3;

    attr_values values1(set1, set2, set3);
    values1.freeze();

    set1["MyAttr"] = attr1;
    attr_values values2(set1, set2, set3);
    values2.freeze();

    set1["MyAttr"] = attr2;
    attr_values values3(set1, set2, set3);
    values3.freeze();

    set1["MyStr"] = attr3;
    attr_values values4(set1, set2, set3);
    values4.freeze();

    // Test with presence filter
    {
        logging::filter f = logging::parse_filter("!%MyAttr%");
        BOOST_CHECK(f(values1));
        BOOST_CHECK(!f(values2));
    }
    {
        logging::filter f = logging::parse_filter(" ! % MyAttr % ");
        BOOST_CHECK(f(values1));
        BOOST_CHECK(!f(values2));
    }
    {
        logging::filter f = logging::parse_filter("not %MyAttr%");
        BOOST_CHECK(f(values1));
        BOOST_CHECK(!f(values2));
    }
    {
        logging::filter f = logging::parse_filter("!!%MyAttr%");
        BOOST_CHECK(!f(values1));
        BOOST_CHECK(f(values2));
    }

    // Test with relations
    {
        logging::filter f = logging::parse_filter("!(%MyAttr% = 10)");
        BOOST_CHECK(f(values1));
        BOOST_CHECK(!f(values2));
        BOOST_CHECK(f(values3));
    }
    {
        logging::filter f = logging::parse_filter("not ( %MyAttr% = 10 ) ");
        BOOST_CHECK(f(values1));
        BOOST_CHECK(!f(values2));
        BOOST_CHECK(f(values3));
    }
    {
        logging::filter f = logging::parse_filter("!(%MyAttr% < 20)");
        BOOST_CHECK(f(values1));
        BOOST_CHECK(!f(values2));
        BOOST_CHECK(f(values3));
    }

    // Test with multiple subexpressions
    {
        logging::filter f = logging::parse_filter("!(%MyAttr% = 20 & %MyStr% = hello)");
        BOOST_CHECK(f(values1));
        BOOST_CHECK(f(values2));
        BOOST_CHECK(f(values3));
        BOOST_CHECK(!f(values4));
    }
    {
        logging::filter f = logging::parse_filter("!(%MyAttr% = 10 | %MyStr% = hello)");
        BOOST_CHECK(f(values1));
        BOOST_CHECK(!f(values2));
        BOOST_CHECK(f(values3));
        BOOST_CHECK(!f(values4));
    }
}

// Tests for begins_with relation filter
BOOST_AUTO_TEST_CASE(begins_with_relation)
{
    attrs::constant< std::string > attr1("abcdABCD");
    attr_set set1, set2, set3;

    set1["MyStr"] = attr1;
    attr_values values1(set1, set2, set3);
    values1.freeze();

    {
        logging::filter f = logging::parse_filter("%MyStr% begins_with \"abcd\"");
        BOOST_CHECK(f(values1));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% begins_with \"ABCD\"");
        BOOST_CHECK(!f(values1));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% begins_with \"efgh\"");
        BOOST_CHECK(!f(values1));
    }
}

// Tests for ends_with relation filter
BOOST_AUTO_TEST_CASE(ends_with_relation)
{
    attrs::constant< std::string > attr1("abcdABCD");
    attr_set set1, set2, set3;

    set1["MyStr"] = attr1;
    attr_values values1(set1, set2, set3);
    values1.freeze();

    {
        logging::filter f = logging::parse_filter("%MyStr% ends_with \"abcd\"");
        BOOST_CHECK(!f(values1));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% ends_with \"ABCD\"");
        BOOST_CHECK(f(values1));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% ends_with \"efgh\"");
        BOOST_CHECK(!f(values1));
    }
}

// Tests for contains relation filter
BOOST_AUTO_TEST_CASE(contains_relation)
{
    attrs::constant< std::string > attr1("abcdABCD");
    attr_set set1, set2, set3;

    set1["MyStr"] = attr1;
    attr_values values1(set1, set2, set3);
    values1.freeze();

    {
        logging::filter f = logging::parse_filter("%MyStr% contains \"abcd\"");
        BOOST_CHECK(f(values1));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% contains \"ABCD\"");
        BOOST_CHECK(f(values1));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% contains \"cdAB\"");
        BOOST_CHECK(f(values1));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% contains \"efgh\"");
        BOOST_CHECK(!f(values1));
    }
}

// Tests for regex matching relation filter
BOOST_AUTO_TEST_CASE(matches_relation)
{
    attrs::constant< std::string > attr1("hello");
    attr_set set1, set2, set3;

    set1["MyStr"] = attr1;
    attr_values values1(set1, set2, set3);
    values1.freeze();

    {
        logging::filter f = logging::parse_filter("%MyStr% matches \"h.*\"");
        BOOST_CHECK(f(values1));
    }
    {
        logging::filter f = logging::parse_filter("%MyStr% matches \"w.*\"");
        BOOST_CHECK(!f(values1));
    }
}

namespace {

class test_filter_factory :
    public logging::filter_factory< char >
{
private:
    typedef logging::filter_factory< char > base_type;

public:
    enum relation_type
    {
        custom,
        exists,
        equality,
        inequality,
        less,
        greater,
        less_or_equal,
        greater_or_equal
    };

    typedef base_type::string_type string_type;

public:
    explicit test_filter_factory(logging::attribute_name const& name) : m_name(name), m_rel(custom), m_called(false)
    {
    }

    void expect_relation(relation_type rel, string_type const& arg)
    {
        m_rel = rel;
        m_arg = arg;
        m_custom_rel.clear();
    }

    void expect_relation(string_type const& rel, string_type const& arg)
    {
        m_rel = custom;
        m_arg = arg;
        m_custom_rel = rel;
    }

    void check_called()
    {
        BOOST_CHECK(m_called);
        m_called = false;
    }

    logging::filter on_exists_test(logging::attribute_name const& name)
    {
        BOOST_CHECK_EQUAL(m_name, name);
        BOOST_CHECK_EQUAL(m_rel, exists);
        m_called = true;
        return logging::filter();
    }
    logging::filter on_equality_relation(logging::attribute_name const& name, string_type const& arg)
    {
        BOOST_CHECK_EQUAL(m_name, name);
        BOOST_CHECK_EQUAL(m_rel, equality);
        BOOST_CHECK_EQUAL(m_arg, arg);
        m_called = true;
        return logging::filter();
    }
    logging::filter on_inequality_relation(logging::attribute_name const& name, string_type const& arg)
    {
        BOOST_CHECK_EQUAL(m_name, name);
        BOOST_CHECK_EQUAL(m_rel, inequality);
        BOOST_CHECK_EQUAL(m_arg, arg);
        m_called = true;
        return logging::filter();
    }
    logging::filter on_less_relation(logging::attribute_name const& name, string_type const& arg)
    {
        BOOST_CHECK_EQUAL(m_name, name);
        BOOST_CHECK_EQUAL(m_rel, less);
        BOOST_CHECK_EQUAL(m_arg, arg);
        m_called = true;
        return logging::filter();
    }
    logging::filter on_greater_relation(logging::attribute_name const& name, string_type const& arg)
    {
        BOOST_CHECK_EQUAL(m_name, name);
        BOOST_CHECK_EQUAL(m_rel, greater);
        BOOST_CHECK_EQUAL(m_arg, arg);
        m_called = true;
        return logging::filter();
    }
    logging::filter on_less_or_equal_relation(logging::attribute_name const& name, string_type const& arg)
    {
        BOOST_CHECK_EQUAL(m_name, name);
        BOOST_CHECK_EQUAL(m_rel, less_or_equal);
        BOOST_CHECK_EQUAL(m_arg, arg);
        m_called = true;
        return logging::filter();
    }
    logging::filter on_greater_or_equal_relation(logging::attribute_name const& name, string_type const& arg)
    {
        BOOST_CHECK_EQUAL(m_name, name);
        BOOST_CHECK_EQUAL(m_rel, greater_or_equal);
        BOOST_CHECK_EQUAL(m_arg, arg);
        m_called = true;
        return logging::filter();
    }
    logging::filter on_custom_relation(logging::attribute_name const& name, string_type const& rel, string_type const& arg)
    {
        BOOST_CHECK_EQUAL(m_name, name);
        BOOST_CHECK_EQUAL(m_rel, custom);
        BOOST_CHECK_EQUAL(m_custom_rel, rel);
        BOOST_CHECK_EQUAL(m_arg, arg);
        m_called = true;
        return logging::filter();
    }

private:
    logging::attribute_name m_name;
    relation_type m_rel;
    string_type m_arg;
    string_type m_custom_rel;
    bool m_called;
};

} // namespace

// Tests for filter factory
BOOST_AUTO_TEST_CASE(filter_factory)
{
    logging::attribute_name attr_name("MyCustomAttr");
    boost::shared_ptr< test_filter_factory > factory(new test_filter_factory(attr_name));
    logging::register_filter_factory(attr_name, factory);

    BOOST_TEST_CHECKPOINT("filter_factory::exists");
    factory->expect_relation(test_filter_factory::exists, "");
    logging::parse_filter("%MyCustomAttr%");
    factory->check_called();

    BOOST_TEST_CHECKPOINT("filter_factory::equality");
    factory->expect_relation(test_filter_factory::equality, "15");
    logging::parse_filter("%MyCustomAttr% = 15");
    factory->check_called();

    BOOST_TEST_CHECKPOINT("filter_factory::equality");
    factory->expect_relation(test_filter_factory::equality, "hello");
    logging::parse_filter("%MyCustomAttr% = hello");
    factory->check_called();

    BOOST_TEST_CHECKPOINT("filter_factory::equality");
    factory->expect_relation(test_filter_factory::equality, "hello");
    logging::parse_filter("%MyCustomAttr% = \"hello\"");
    factory->check_called();

    BOOST_TEST_CHECKPOINT("filter_factory::equality");
    factory->expect_relation(test_filter_factory::equality, "hello\nworld");
    logging::parse_filter("%MyCustomAttr% = \"hello\\nworld\"");
    factory->check_called();

    BOOST_TEST_CHECKPOINT("filter_factory::inequality");
    factory->expect_relation(test_filter_factory::inequality, "hello");
    logging::parse_filter("%MyCustomAttr% != \"hello\"");
    factory->check_called();

    BOOST_TEST_CHECKPOINT("filter_factory::less");
    factory->expect_relation(test_filter_factory::less, "hello");
    logging::parse_filter("%MyCustomAttr% < \"hello\"");
    factory->check_called();

    BOOST_TEST_CHECKPOINT("filter_factory::greater");
    factory->expect_relation(test_filter_factory::greater, "hello");
    logging::parse_filter("%MyCustomAttr% > \"hello\"");
    factory->check_called();

    BOOST_TEST_CHECKPOINT("filter_factory::less_or_equal");
    factory->expect_relation(test_filter_factory::less_or_equal, "hello");
    logging::parse_filter("%MyCustomAttr% <= \"hello\"");
    factory->check_called();

    BOOST_TEST_CHECKPOINT("filter_factory::greater_or_equal");
    factory->expect_relation(test_filter_factory::greater_or_equal, "hello");
    logging::parse_filter("%MyCustomAttr% >= \"hello\"");
    factory->check_called();

    BOOST_TEST_CHECKPOINT("filter_factory::custom");
    factory->expect_relation("my_relation", "hello");
    logging::parse_filter("%MyCustomAttr% my_relation \"hello\"");
    factory->check_called();
}

// Tests for invalid filters
BOOST_AUTO_TEST_CASE(invalid)
{
    BOOST_CHECK_THROW(logging::parse_filter("%MyStr"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_filter("MyStr%"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_filter("%MyStr% abcd"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_filter("(%MyStr%"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_filter("%MyStr%)"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_filter("%%"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_filter("%"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_filter("!"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_filter("!()"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_filter("\"xxx\" == %MyStr%"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_filter("%MyStr% == \"xxx"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_filter("%MyStr% === \"xxx\""), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_filter("%MyStr% ! \"xxx\""), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_filter("%MyStr% %MyStr2%"), logging::parse_error);
}

#endif // !defined(BOOST_LOG_WITHOUT_SETTINGS_PARSERS) && !defined(BOOST_LOG_WITHOUT_DEFAULT_FACTORIES)
