/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   form_attr.cpp
 * \author Andrey Semashev
 * \date   01.02.2009
 *
 * \brief  This header contains tests for the \c attr formatter.
 */

#define BOOST_TEST_MODULE form_attr

#include <memory>
#include <string>
#include <iomanip>
#include <ostream>
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/utility/type_dispatch/standard_types.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/core/record.hpp>
#include "char_definitions.hpp"
#include "make_record.hpp"

namespace logging = boost::log;
namespace attrs = logging::attributes;
namespace expr = logging::expressions;

namespace {

    class my_class
    {
        int m_Data;

    public:
        explicit my_class(int data) : m_Data(data) {}

        template< typename CharT, typename TraitsT >
        friend std::basic_ostream< CharT, TraitsT >& operator<< (std::basic_ostream< CharT, TraitsT >& strm, my_class const& obj)
        {
            strm << "[data: " << obj.m_Data << "]";
            return strm;
        }
    };

} // namespace

// The test checks that default formatting work
BOOST_AUTO_TEST_CASE_TEMPLATE(default_formatting, CharT, char_types)
{
    typedef logging::record_view record_view;
    typedef logging::attribute_set attr_set;
    typedef std::basic_string< CharT > string;
    typedef logging::basic_formatting_ostream< CharT > osstream;
    typedef logging::basic_formatter< CharT > formatter;
    typedef test_data< CharT > data;

    attrs::constant< int > attr1(10);
    attrs::constant< double > attr2(5.5);
    attrs::constant< my_class > attr3(my_class(77));

    attr_set set1;
    set1[data::attr1()] = attr1;
    set1[data::attr2()] = attr2;
    set1[data::attr3()] = attr3;

    record_view rec = make_record_view(set1);

    // Check for various modes of attribute value type specification
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::attr< int >(data::attr1()) << expr::attr< double >(data::attr2());
        f(rec, strm1);
        strm2 << 10 << 5.5;
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::attr< logging::numeric_types >(data::attr1()) << expr::attr< logging::numeric_types >(data::attr2());
        f(rec, strm1);
        strm2 << 10 << 5.5;
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
    // Check that custom types as attribute values are also supported
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::attr< my_class >(data::attr3());
        f(rec, strm1);
        strm2 << my_class(77);
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
    // Check how missing attribute values are handled
    {
        string str1;
        osstream strm1(str1);
        formatter f = expr::stream
            << expr::attr< int >(data::attr1())
            << expr::attr< int >(data::attr4()).or_throw()
            << expr::attr< double >(data::attr2());
        BOOST_CHECK_THROW(f(rec, strm1), logging::runtime_error);
    }
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream
            << expr::attr< int >(data::attr1())
            << expr::attr< int >(data::attr4())
            << expr::attr< double >(data::attr2());
        f(rec, strm1);
        strm2 << 10 << 5.5;
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
}
