//  //  Unit test for boost::lexical_cast.
//
//  See http://www.boost.org for most recent version, including documentation.
//
//  Copyright Antony Polukhin, 2013.
//
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).

#include <boost/lexical_cast.hpp>
#include <boost/type.hpp>

#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

int test_main(int, char*[])
{
    boost::lexical_cast<char*>("Hello");
    BOOST_CHECK(false); // suppressing warning about 'boost::unit_test::{anonymous}::unit_test_log' defined but not used
    return 0;
}

