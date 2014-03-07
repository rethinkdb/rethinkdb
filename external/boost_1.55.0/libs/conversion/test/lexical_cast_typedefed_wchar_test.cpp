//  Unit test for boost::lexical_cast.
//
//  See http://www.boost.org for most recent version, including documentation.
//
//  Copyright Antony Polukhin, 2011.
//
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).

#include <boost/config.hpp>

#include <boost/static_assert.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

void parseDate()
{
  std::locale locale;
  boost::date_time::format_date_parser<boost::gregorian::date, wchar_t> parser(L"", locale);
  boost::date_time::special_values_parser<boost::gregorian::date, wchar_t> svp;

  boost::gregorian::date date = parser.parse_date(L"", L"", svp);
  (void)date;
}


int main()
{
#ifdef BOOST_MSVC
    BOOST_STATIC_ASSERT((boost::is_same<wchar_t, unsigned short>::value));
#endif

    parseDate();
    return ::boost::lexical_cast<int>(L"1000") == 1000;
}


