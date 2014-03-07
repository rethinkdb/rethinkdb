/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   form_named_scope.cpp
 * \author Andrey Semashev
 * \date   07.02.2009
 *
 * \brief  This header contains tests for the \c named_scope formatter.
 */

#define BOOST_TEST_MODULE form_named_scope

#include <string>
#include <boost/preprocessor/cat.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/string_literal.hpp>
#include <boost/log/core/record.hpp>
#include "char_definitions.hpp"
#include "make_record.hpp"

namespace logging = boost::log;
namespace attrs = logging::attributes;
namespace expr = logging::expressions;
namespace keywords = logging::keywords;

namespace {

    template< typename CharT >
    struct named_scope_test_data;

#ifdef BOOST_LOG_USE_CHAR
    template< >
    struct named_scope_test_data< char > :
        public test_data< char >
    {
        static logging::string_literal default_format() { return logging::str_literal("%n"); }
        static logging::string_literal full_format() { return logging::str_literal("%n (%f:%l)"); }
        static logging::string_literal delimiter1() { return logging::str_literal("|"); }

        static logging::string_literal scope1() { return logging::str_literal("scope1"); }
        static logging::string_literal scope2() { return logging::str_literal("scope2"); }
        static logging::string_literal file() { return logging::str_literal(__FILE__); }
    };
#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T
    template< >
    struct named_scope_test_data< wchar_t > :
        public test_data< wchar_t >
    {
        static logging::wstring_literal default_format() { return logging::str_literal(L"%n"); }
        static logging::wstring_literal full_format() { return logging::str_literal(L"%n (%f:%l)"); }
        static logging::wstring_literal delimiter1() { return logging::str_literal(L"|"); }

        static logging::string_literal scope1() { return logging::str_literal("scope1"); }
        static logging::string_literal scope2() { return logging::str_literal("scope2"); }
        static logging::string_literal file() { return logging::str_literal(__FILE__); }
    };
#endif // BOOST_LOG_USE_WCHAR_T

} // namespace

// The test checks that named scopes stack formatting works
BOOST_AUTO_TEST_CASE_TEMPLATE(scopes_formatting, CharT, char_types)
{
    typedef attrs::named_scope named_scope;
    typedef named_scope::sentry sentry;
    typedef attrs::named_scope_list scopes;
    typedef attrs::named_scope_entry scope;

    typedef logging::attribute_set attr_set;
    typedef std::basic_string< CharT > string;
    typedef logging::basic_formatting_ostream< CharT > osstream;
    typedef logging::record_view record_view;
    typedef logging::basic_formatter< CharT > formatter;
    typedef named_scope_test_data< CharT > data;

    named_scope attr;

    // First scope
    const unsigned int line1 = __LINE__;
    sentry scope1(data::scope1(), data::file(), line1);
    const unsigned int line2 = __LINE__;
    sentry scope2(data::scope2(), data::file(), line2);

    attr_set set1;
    set1[data::attr1()] = attr;

    record_view rec = make_record_view(set1);

    // Default format
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::format_named_scope(data::attr1(),
            keywords::format = data::default_format().c_str());
        f(rec, strm1);
        strm2 << data::scope1() << "->" << data::scope2();
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
    // Full format
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::format_named_scope(data::attr1(),
            keywords::format = data::full_format().c_str());
        f(rec, strm1);
        strm2 << data::scope1() << " (" << data::file() << ":" << line1 << ")->"
              << data::scope2() << " (" << data::file() << ":" << line2 << ")";
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
    // Different delimiter
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::format_named_scope(data::attr1(),
            keywords::format = data::default_format().c_str(),
            keywords::delimiter = data::delimiter1().c_str());
        f(rec, strm1);
        strm2 << data::scope1() << "|" << data::scope2();
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
    // Different direction
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::format_named_scope(data::attr1(),
            keywords::format = data::default_format().c_str(),
            keywords::iteration = expr::reverse);
        f(rec, strm1);
        strm2 << data::scope2() << "<-" << data::scope1();
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::format_named_scope(data::attr1(),
            keywords::format = data::default_format().c_str(),
            keywords::delimiter = data::delimiter1().c_str(),
            keywords::iteration = expr::reverse);
        f(rec, strm1);
        strm2 << data::scope2() << "|" << data::scope1();
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
    // Limiting the number of scopes
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::format_named_scope(data::attr1(),
            keywords::format = data::default_format().c_str(),
            keywords::depth = 1);
        f(rec, strm1);
        strm2 << "...->" << data::scope2();
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::format_named_scope(data::attr1(),
            keywords::format = data::default_format().c_str(),
            keywords::depth = 1,
            keywords::iteration = expr::reverse);
        f(rec, strm1);
        strm2 << data::scope2() << "<-...";
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::format_named_scope(data::attr1(),
            keywords::format = data::default_format().c_str(),
            keywords::delimiter = data::delimiter1().c_str(),
            keywords::depth = 1);
        f(rec, strm1);
        strm2 << "...|" << data::scope2();
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::format_named_scope(data::attr1(),
            keywords::format = data::default_format().c_str(),
            keywords::delimiter = data::delimiter1().c_str(),
            keywords::depth = 1,
            keywords::iteration = expr::reverse);
        f(rec, strm1);
        strm2 << data::scope2() << "|...";
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
}
