/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   form_if.cpp
 * \author Andrey Semashev
 * \date   05.02.2009
 *
 * \brief  This header contains tests for the \c if_ formatter.
 */

#define BOOST_TEST_MODULE form_if

#include <string>
#include <ostream>
#include <boost/test/unit_test.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include "char_definitions.hpp"
#include "make_record.hpp"

namespace logging = boost::log;
namespace attrs = logging::attributes;
namespace expr = logging::expressions;

// The test checks that conditional formatting work
BOOST_AUTO_TEST_CASE_TEMPLATE(conditional_formatting, CharT, char_types)
{
    typedef logging::attribute_set attr_set;
    typedef std::basic_string< CharT > string;
    typedef logging::basic_formatting_ostream< CharT > osstream;
    typedef logging::record_view record_view;
    typedef logging::basic_formatter< CharT > formatter;
    typedef test_data< CharT > data;

    attrs::constant< int > attr1(10);
    attrs::constant< double > attr2(5.5);

    attr_set set1;
    set1[data::attr1()] = attr1;
    set1[data::attr2()] = attr2;

    record_view rec = make_record_view(set1);

    // Check for various modes of attribute value type specification
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream <<
            expr::if_(expr::has_attr< int >(data::attr1()))
            [
                expr::stream << expr::attr< int >(data::attr1())
            ];
        f(rec, strm1);
        strm2 << 10;
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
    {
        string str1;
        osstream strm1(str1);
        formatter f = expr::stream <<
            expr::if_(expr::has_attr< int >(data::attr2()))
            [
                expr::stream << expr::attr< int >(data::attr2())
            ];
        f(rec, strm1);
        BOOST_CHECK(equal_strings(strm1.str(), string()));
    }
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream <<
            expr::if_(expr::has_attr< int >(data::attr1()))
            [
                expr::stream << expr::attr< int >(data::attr1())
            ]
            .else_
            [
                expr::stream << expr::attr< double >(data::attr2())
            ];
        f(rec, strm1);
        strm2 << 10;
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream <<
            expr::if_(expr::has_attr< int >(data::attr2()))
            [
                expr::stream << expr::attr< int >(data::attr1())
            ]
            .else_
            [
                expr::stream << expr::attr< double >(data::attr2())
            ];
        f(rec, strm1);
        strm2 << 5.5;
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
}
