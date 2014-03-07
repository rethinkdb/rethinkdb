/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   form_date_time.cpp
 * \author Andrey Semashev
 * \date   07.02.2009
 *
 * \brief  This header contains tests for the date and time formatters.
 */

#define BOOST_TEST_MODULE form_date_time

#include <memory>
#include <locale>
#include <string>
#include <iomanip>
#include <ostream>
#include <algorithm>
#include <boost/date_time.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/utility/string_literal.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/support/date_time.hpp>
#include "char_definitions.hpp"
#include "make_record.hpp"

namespace logging = boost::log;
namespace attrs = logging::attributes;
namespace expr = logging::expressions;
namespace keywords = logging::keywords;

typedef boost::posix_time::ptime ptime;
typedef boost::gregorian::date gdate;
typedef ptime::time_duration_type duration;

namespace {

    template< typename CharT >
    struct date_time_formats;

#ifdef BOOST_LOG_USE_CHAR
    template< >
    struct date_time_formats< char >
    {
        typedef logging::basic_string_literal< char > string_literal_type;

        static string_literal_type default_date_format() { return logging::str_literal("%Y-%b-%d"); }
        static string_literal_type default_time_format() { return logging::str_literal("%H:%M:%S.%f"); }
        static string_literal_type default_date_time_format() { return logging::str_literal("%Y-%b-%d %H:%M:%S.%f"); }
        static string_literal_type default_time_duration_format() { return logging::str_literal("%-%H:%M:%S.%f"); }

        static string_literal_type date_format() { return logging::str_literal("%d/%m/%Y"); }
        static string_literal_type time_format() { return logging::str_literal("%H.%M.%S"); }
        static string_literal_type date_time_format() { return logging::str_literal("%d/%m/%Y %H.%M.%S"); }
        static string_literal_type time_duration_format() { return logging::str_literal("%+%H.%M.%S.%f"); }
    };
#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T
    template< >
    struct date_time_formats< wchar_t >
    {
        typedef logging::basic_string_literal< wchar_t > string_literal_type;

        static string_literal_type default_date_format() { return logging::str_literal(L"%Y-%b-%d"); }
        static string_literal_type default_time_format() { return logging::str_literal(L"%H:%M:%S.%f"); }
        static string_literal_type default_date_time_format() { return logging::str_literal(L"%Y-%b-%d %H:%M:%S.%f"); }
        static string_literal_type default_time_duration_format() { return logging::str_literal(L"%-%H:%M:%S.%f"); }

        static string_literal_type date_format() { return logging::str_literal(L"%d/%m/%Y"); }
        static string_literal_type time_format() { return logging::str_literal(L"%H.%M.%S"); }
        static string_literal_type date_time_format() { return logging::str_literal(L"%d/%m/%Y %H.%M.%S"); }
        static string_literal_type time_duration_format() { return logging::str_literal(L"%+%H.%M.%S.%f"); }
    };
#endif // BOOST_LOG_USE_WCHAR_T

} // namespace

// The test checks that date_time formatting work
BOOST_AUTO_TEST_CASE_TEMPLATE(date_time, CharT, char_types)
{
    typedef logging::attribute_set attr_set;
    typedef std::basic_string< CharT > string;
    typedef logging::basic_formatting_ostream< CharT > osstream;
    typedef logging::record_view record_view;
    typedef logging::basic_formatter< CharT > formatter;
    typedef test_data< CharT > data;
    typedef date_time_formats< CharT > formats;
    typedef boost::date_time::time_facet< ptime, CharT > facet;

    ptime t1(gdate(2009, 2, 7), ptime::time_duration_type(14, 40, 15));
    attrs::constant< ptime > attr1(t1);

    attr_set set1;
    set1[data::attr1()] = attr1;

    record_view rec = make_record_view(set1);

    // Check for various formats specification
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::format_date_time< ptime >(data::attr1(), formats::default_date_time_format().c_str());
        f(rec, strm1);
        strm2.imbue(std::locale(strm2.getloc(), new facet(formats::default_date_time_format().c_str())));
        strm2 << t1;
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::format_date_time< ptime >(data::attr1(), formats::date_time_format().c_str());
        f(rec, strm1);
        strm2.imbue(std::locale(strm2.getloc(), new facet(formats::date_time_format().c_str())));
        strm2 << t1;
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
}

// The test checks that date formatting work
BOOST_AUTO_TEST_CASE_TEMPLATE(date, CharT, char_types)
{
    typedef logging::attribute_set attr_set;
    typedef std::basic_string< CharT > string;
    typedef logging::basic_formatting_ostream< CharT > osstream;
    typedef logging::record_view record_view;
    typedef logging::basic_formatter< CharT > formatter;
    typedef test_data< CharT > data;
    typedef date_time_formats< CharT > formats;
    typedef boost::date_time::date_facet< gdate, CharT > facet;

    gdate d1(2009, 2, 7);
    attrs::constant< gdate > attr1(d1);

    attr_set set1;
    set1[data::attr1()] = attr1;

    record_view rec = make_record_view(set1);

    // Check for various formats specification
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::format_date_time< gdate >(data::attr1(), formats::default_date_format().c_str());
        f(rec, strm1);
        strm2.imbue(std::locale(strm2.getloc(), new facet(formats::default_date_format().c_str())));
        strm2 << d1;
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::format_date_time< gdate >(data::attr1(), formats::date_format().c_str());
        f(rec, strm1);
        strm2.imbue(std::locale(strm2.getloc(), new facet(formats::date_format().c_str())));
        strm2 << d1;
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
}

// The test checks that time_duration formatting work
BOOST_AUTO_TEST_CASE_TEMPLATE(time_duration, CharT, char_types)
{
    typedef logging::attribute_set attr_set;
    typedef std::basic_string< CharT > string;
    typedef logging::basic_formatting_ostream< CharT > osstream;
    typedef logging::record_view record_view;
    typedef logging::basic_formatter< CharT > formatter;
    typedef test_data< CharT > data;
    typedef date_time_formats< CharT > formats;
    typedef boost::date_time::time_facet< ptime, CharT > facet;

    duration t1(14, 40, 15);
    attrs::constant< duration > attr1(t1);

    attr_set set1;
    set1[data::attr1()] = attr1;

    record_view rec = make_record_view(set1);

    // Check for various formats specification
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::format_date_time< duration >(data::attr1(), formats::default_time_duration_format().c_str());
        f(rec, strm1);
        facet* fac = new facet();
        fac->time_duration_format(formats::default_time_duration_format().c_str());
        strm2.imbue(std::locale(strm2.getloc(), fac));
        strm2 << t1;
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
    {
        string str1, str2;
        osstream strm1(str1), strm2(str2);
        formatter f = expr::stream << expr::format_date_time< duration >(data::attr1(), formats::time_duration_format().c_str());
        f(rec, strm1);
        facet* fac = new facet();
        fac->time_duration_format(formats::time_duration_format().c_str());
        strm2.imbue(std::locale(strm2.getloc(), fac));
        strm2 << t1;
        BOOST_CHECK(equal_strings(strm1.str(), strm2.str()));
    }
}
