/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   form_message.cpp
 * \author Andrey Semashev
 * \date   01.02.2009
 *
 * \brief  This header contains tests for the \c message formatter.
 */

#define BOOST_TEST_MODULE form_message

#include <string>
#include <boost/test/unit_test.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/core/record.hpp>
#include "char_definitions.hpp"
#include "make_record.hpp"

namespace logging = boost::log;
namespace attrs = logging::attributes;
namespace expr = logging::expressions;

namespace {

    template< typename CharT >
    struct message_test_data;

#ifdef BOOST_LOG_USE_CHAR
    template< >
    struct message_test_data< char > :
        public test_data< char >
    {
        static expr::smessage_type message() { return expr::smessage; }
    };
#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T
    template< >
    struct message_test_data< wchar_t > :
        public test_data< wchar_t >
    {
        static expr::wmessage_type message() { return expr::wmessage; }
    };
#endif // BOOST_LOG_USE_WCHAR_T

} // namespace

// The test checks that message formatting work
BOOST_AUTO_TEST_CASE_TEMPLATE(message_formatting, CharT, char_types)
{
    typedef logging::attribute_set attr_set;
    typedef std::basic_string< CharT > string;
    typedef logging::basic_formatting_ostream< CharT > osstream;
    typedef logging::record_view record_view;
    typedef logging::basic_formatter< CharT > formatter;
    typedef message_test_data< CharT > data;

    attrs::constant< int > attr1(10);
    attrs::constant< double > attr2(5.5);

    attr_set set1;
    set1[data::attr1()] = attr1;
    set1[data::attr2()] = attr2;
    set1[data::message().get_name()] = attrs::constant< string >(data::some_test_string());

    record_view rec = make_record_view(set1);

    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << data::message();
        f(rec, strm1);
        strm2 << data::some_test_string();
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
}
