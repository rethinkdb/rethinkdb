//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//This example shows how to add data to boost::exception objects at the
//point of the throw, and how to retrieve that data at the point of the catch.

#include <boost/exception/all.hpp>
#include <iostream>

typedef boost::error_info<struct tag_my_info,int> my_info; //(1)

struct my_error: virtual boost::exception, virtual std::exception { }; //(2)

void
f()
    {
    throw my_error() << my_info(42); //(3)
    }
                 
void
g()
    {
    try
        {
        f();
        }
    catch(
    my_error & x )
        {
        if( int const * mi=boost::get_error_info<my_info>(x) )
            std::cerr << "My info: " << *mi;
        }
    }
