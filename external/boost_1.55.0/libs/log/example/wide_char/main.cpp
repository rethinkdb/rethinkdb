/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   main.cpp
 * \author Andrey Semashev
 * \date   01.12.2012
 *
 * \brief  An example of wide character logging.
 */

#include <iostream>
#include <boost/locale/generator.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/support/date_time.hpp>

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

//[ example_wide_char_severity_level_definition
enum severity_level
{
    normal,
    notification,
    warning,
    error,
    critical
};

template< typename CharT, typename TraitsT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (
    std::basic_ostream< CharT, TraitsT >& strm, severity_level lvl)
{
    static const char* const str[] =
    {
        "normal",
        "notification",
        "warning",
        "error",
        "critical"
    };
    if (static_cast< std::size_t >(lvl) < (sizeof(str) / sizeof(*str)))
        strm << str[lvl];
    else
        strm << static_cast< int >(lvl);
    return strm;
}
//]

//[ example_wide_char_logging_initialization
// Declare attribute keywords
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

void init_logging()
{
    boost::shared_ptr< sinks::synchronous_sink< sinks::text_file_backend > > sink = logging::add_file_log
    (
        "sample.log",
        keywords::format = expr::stream
            << expr::format_date_time(timestamp, "%Y-%m-%d, %H:%M:%S.%f")
            << " <" << severity.or_default(normal)
            << "> " << expr::message
    );

    // The sink will perform character code conversion as needed, according to the locale set with imbue()
    std::locale loc = boost::locale::generator()("en_US.UTF-8");
    sink->imbue(loc);

    // Let's add some commonly used attributes, like timestamp and record counter.
    logging::add_common_attributes();
}
//]

//[ example_wide_char_logging
void test_narrow_char_logging()
{
    // Narrow character logging still works
    src::logger lg;
    BOOST_LOG(lg) << "Hello, World! This is a narrow character message.";
}

void test_wide_char_logging()
{
    src::wlogger lg;
    BOOST_LOG(lg) << L"Hello, World! This is a wide character message.";

    // National characters are also supported
    const wchar_t national_chars[] = { 0x041f, 0x0440, 0x0438, 0x0432, 0x0435, 0x0442, L',', L' ', 0x043c, 0x0438, 0x0440, L'!', 0 };
    BOOST_LOG(lg) << national_chars;

    // Now, let's try logging with severity
    src::wseverity_logger< severity_level > slg;
    BOOST_LOG_SEV(slg, normal) << L"A normal severity message, will not pass to the file";
    BOOST_LOG_SEV(slg, warning) << L"A warning severity message, will pass to the file";
    BOOST_LOG_SEV(slg, error) << L"An error severity message, will pass to the file";
}
//]

int main(int argc, char* argv[])
{
    init_logging();
    test_narrow_char_logging();
    test_wide_char_logging();

    return 0;
}
