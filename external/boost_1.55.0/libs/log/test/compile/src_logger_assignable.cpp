/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   src_logger_assignable.cpp
 * \author Andrey Semashev
 * \date   16.05.2011
 *
 * \brief  This header contains a test for logger assignability.
 */

#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/channel_logger.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>

template< typename LoggerT >
void test()
{
    LoggerT lg1, lg2;

    // Loggers must be assignable. The assignment operator must be taken
    // from the composite_logger class and not auto-generated (in which
    // case it will fail to compile because assignment in basic_logger is private).
    lg1 = lg2;
}

int main(int, char*[])
{
    test< boost::log::sources::logger >();
    test< boost::log::sources::severity_logger< > >();
    test< boost::log::sources::channel_logger< > >();
    test< boost::log::sources::severity_channel_logger< > >();

    return 0;
}
