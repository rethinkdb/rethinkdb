/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   setup_formatter_parser.cpp
 * \author Andrey Semashev
 * \date   25.08.2013
 *
 * \brief  This header contains tests for the formatter parser.
 */

#define BOOST_TEST_MODULE setup_formatter_parser

#include <string>
#include <boost/test/unit_test.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>

#if !defined(BOOST_LOG_WITHOUT_SETTINGS_PARSERS) && !defined(BOOST_LOG_WITHOUT_DEFAULT_FACTORIES)

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/core/record_view.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/attribute_value_set.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/expressions/formatter.hpp>
#include "make_record.hpp"

namespace logging = boost::log;
namespace attrs = logging::attributes;

typedef logging::attribute_set attr_set;
typedef logging::attribute_value_set attr_values;
typedef logging::record_view record_view;

typedef logging::basic_formatting_ostream< char > osstream;
typedef logging::basic_formatter< char > formatter;

// Tests for simple attribute placeholders in formatters
BOOST_AUTO_TEST_CASE(attr_placeholders)
{
    attrs::constant< int > attr1(10);
    attrs::constant< std::string > attr2("hello");
    attr_set set1;

    set1["MyAttr"] = attr1;
    set1["MyStr"] = attr2;

    record_view rec = make_record_view(set1);

    {
        formatter f = logging::parse_formatter("String literal");
        std::string str1, str2;
        osstream strm1(str1), strm2(str2);
        f(rec, strm1);
        strm2 << "String literal";
        BOOST_CHECK_EQUAL(strm1.str(), strm2.str());
    }
    {
        formatter f = logging::parse_formatter("MyAttr: %MyAttr%, MyStr: %MyStr%");
        std::string str1, str2;
        osstream strm1(str1), strm2(str2);
        f(rec, strm1);
        strm2 << "MyAttr: " << attr1.get() << ", MyStr: " << attr2.get();
        BOOST_CHECK_EQUAL(strm1.str(), strm2.str());
    }
    {
        formatter f = logging::parse_formatter("MyAttr: % MyAttr %, MyStr: % MyStr %");
        std::string str1, str2;
        osstream strm1(str1), strm2(str2);
        f(rec, strm1);
        strm2 << "MyAttr: " << attr1.get() << ", MyStr: " << attr2.get();
        BOOST_CHECK_EQUAL(strm1.str(), strm2.str());
    }
    {
        formatter f = logging::parse_formatter("%MyAttr%%MyStr%");
        std::string str1, str2;
        osstream strm1(str1), strm2(str2);
        f(rec, strm1);
        strm2 << attr1.get() << attr2.get();
        BOOST_CHECK_EQUAL(strm1.str(), strm2.str());
    }
    {
        formatter f = logging::parse_formatter("MyAttr: %MyAttr%, MissingAttr: %MissingAttr%");
        std::string str1, str2;
        osstream strm1(str1), strm2(str2);
        f(rec, strm1);
        strm2 << "MyAttr: " << attr1.get() << ", MissingAttr: ";
        BOOST_CHECK_EQUAL(strm1.str(), strm2.str());
    }
}

namespace {

class test_formatter_factory :
    public logging::formatter_factory< char >
{
private:
    typedef logging::formatter_factory< char > base_type;

public:
    typedef base_type::string_type string_type;
    typedef base_type::args_map args_map;
    typedef base_type::formatter_type formatter_type;

public:
    explicit test_formatter_factory(logging::attribute_name const& name) : m_name(name), m_called(false)
    {
    }

    void expect_args(args_map const& args)
    {
        m_args = args;
    }

    void check_called()
    {
        BOOST_CHECK(m_called);
        m_called = false;
    }

    formatter_type create_formatter(logging::attribute_name const& name, args_map const& args)
    {
        BOOST_CHECK_EQUAL(m_name, name);
        BOOST_CHECK_EQUAL(m_args.size(), args.size());

        for (args_map::const_iterator it = m_args.begin(), end = m_args.end(); it != end; ++it)
        {
            args_map::const_iterator parsed_it = args.find(it->first);
            if (parsed_it != args.end())
            {
                BOOST_TEST_CHECKPOINT(("Arg: " + it->first));
                BOOST_CHECK_EQUAL(it->second, parsed_it->second);
            }
            else
            {
                BOOST_ERROR(("Arg not found: " + it->first));
            }
        }

        m_called = true;
        return formatter_type();
    }

private:
    logging::attribute_name m_name;
    args_map m_args;
    bool m_called;
};

} // namespace

// Tests for formatter factory
BOOST_AUTO_TEST_CASE(formatter_factory)
{
    logging::attribute_name attr_name("MyCustomAttr");
    boost::shared_ptr< test_formatter_factory > factory(new test_formatter_factory(attr_name));
    logging::register_formatter_factory(attr_name, factory);

    {
        BOOST_TEST_CHECKPOINT("formatter 1");
        test_formatter_factory::args_map args;
        factory->expect_args(args);
        logging::parse_formatter("Hello: %MyCustomAttr%");
        factory->check_called();
    }
    {
        BOOST_TEST_CHECKPOINT("formatter 2");
        test_formatter_factory::args_map args;
        factory->expect_args(args);
        logging::parse_formatter("Hello: %MyCustomAttr()%");
        factory->check_called();
    }
    {
        BOOST_TEST_CHECKPOINT("formatter 3");
        test_formatter_factory::args_map args;
        factory->expect_args(args);
        logging::parse_formatter("Hello: %MyCustomAttr ( ) %");
        factory->check_called();
    }
    {
        BOOST_TEST_CHECKPOINT("formatter 4");
        test_formatter_factory::args_map args;
        args["param1"] = "10";
        factory->expect_args(args);
        logging::parse_formatter("Hello: %MyCustomAttr(param1=10)%");
        factory->check_called();
    }
    {
        BOOST_TEST_CHECKPOINT("formatter 5");
        test_formatter_factory::args_map args;
        args["param1"] = "10";
        factory->expect_args(args);
        logging::parse_formatter("Hello: % MyCustomAttr ( param1 = 10 ) % ");
        factory->check_called();
    }
    {
        BOOST_TEST_CHECKPOINT("formatter 6");
        test_formatter_factory::args_map args;
        args["param1"] = " 10 ";
        factory->expect_args(args);
        logging::parse_formatter("Hello: %MyCustomAttr(param1=\" 10 \")%");
        factory->check_called();
    }
    {
        BOOST_TEST_CHECKPOINT("formatter 7");
        test_formatter_factory::args_map args;
        args["param1"] = "10";
        args["param2"] = "abcd";
        factory->expect_args(args);
        logging::parse_formatter("Hello: %MyCustomAttr(param1 = 10, param2 = abcd)%");
        factory->check_called();
    }
    {
        BOOST_TEST_CHECKPOINT("formatter 8");
        test_formatter_factory::args_map args;
        args["param1"] = "10";
        args["param2"] = "abcd";
        factory->expect_args(args);
        logging::parse_formatter("Hello: %MyCustomAttr(param1=10,param2=abcd)%");
        factory->check_called();
    }
    {
        BOOST_TEST_CHECKPOINT("formatter 9");
        test_formatter_factory::args_map args;
        args["param1"] = "10";
        args["param2"] = "abcd";
        args["param_last"] = "-2.2";
        factory->expect_args(args);
        logging::parse_formatter("Hello: %MyCustomAttr(param1 = 10, param2 = \"abcd\", param_last = -2.2)%");
        factory->check_called();
    }
}

// Tests for invalid formatters
BOOST_AUTO_TEST_CASE(invalid)
{
    BOOST_CHECK_THROW(logging::parse_formatter("%MyStr"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_formatter("MyStr%"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_formatter("%MyStr(%"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_formatter("%MyStr)%"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_formatter("%MyStr(param%"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_formatter("%MyStr(param=%"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_formatter("%MyStr(param)%"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_formatter("%MyStr(param=)%"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_formatter("%MyStr(=value)%"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_formatter("%MyStr(param,param2)%"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_formatter("%MyStr(param=value,)%"), logging::parse_error);
    BOOST_CHECK_THROW(logging::parse_formatter("%MyStr(param=value,param2)%"), logging::parse_error);
}

#endif // !defined(BOOST_LOG_WITHOUT_SETTINGS_PARSERS) && !defined(BOOST_LOG_WITHOUT_DEFAULT_FACTORIES)
