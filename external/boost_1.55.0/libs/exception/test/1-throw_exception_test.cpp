//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/throw_exception.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/config.hpp>

class my_exception: public std::exception { };

int
main()
    {
    try
        {
        boost::throw_exception(my_exception());
        BOOST_ERROR("boost::throw_exception failed to throw.");
        }
    catch(
    my_exception & )
        {
        }
    catch(
    ... )
        {
        BOOST_ERROR("boost::throw_exception malfunction.");
        }
    return boost::report_errors();
    }
