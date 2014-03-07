/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   util_unique_identifier.cpp
 * \author Andrey Semashev
 * \date   24.01.2009
 *
 * \brief  This header contains tests for the unique identifier name generator.
 */

#include <boost/log/utility/unique_identifier_name.hpp>

int main(int, char*[])
{
    // Names with the same prefixes may coexist in different lines
    int BOOST_LOG_UNIQUE_IDENTIFIER_NAME(var) = 0;
    int BOOST_LOG_UNIQUE_IDENTIFIER_NAME(var) = 0;

    // Names with different prefixes may coexist on the same line
    int BOOST_LOG_UNIQUE_IDENTIFIER_NAME(var1) = 0; int BOOST_LOG_UNIQUE_IDENTIFIER_NAME(var2) = 0;

    return 0;
}
