//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/exception/current_exception_cast.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <exception>

class
my_exception:
    public std::exception
    {
    };

class
polymorphic
    {
    virtual
    ~polymorphic()
        {
        }
    };

int
main()
    {
    try
        {
        throw my_exception();
        }
    catch(
    std::exception & e )
        {
        try
            {
            throw;
            }
        catch(
        ...)
            {
            BOOST_TEST(boost::current_exception_cast<std::exception>()==&e);
            BOOST_TEST(!boost::current_exception_cast<polymorphic>());
            }
        }
    return boost::report_errors();
    }
