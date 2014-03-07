//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_NO_EXCEPTIONS
#include <boost/throw_exception.hpp>
#include <boost/detail/lightweight_test.hpp>

class my_exception: public std::exception { };

bool called=false;

namespace
boost
    {
    void
    throw_exception( std::exception const & )
        {
        called=true;
        }
    }

int
main()
    {
    boost::throw_exception(my_exception());
    BOOST_TEST(called);
    return boost::report_errors();
    }
