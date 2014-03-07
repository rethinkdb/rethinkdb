//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/exception/exception.hpp>
#include <boost/detail/lightweight_test.hpp>

class
test_exception:
    public boost::exception
    {
    };

void
test_throw()
    {
    throw test_exception();
    }

int
main()
    {
    try
        {
        test_throw();
        BOOST_TEST(false);
        }
    catch(
    test_exception & )
        {
        BOOST_TEST(true);
        }
    return boost::report_errors();
    }
