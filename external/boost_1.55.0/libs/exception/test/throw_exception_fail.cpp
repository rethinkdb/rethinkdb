//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/throw_exception.hpp>

struct
my_exception
    {
    };

void
tester()
    {
    //Must not compile, throw_exception requires exception types to derive std::exception.
    boost::throw_exception(my_exception());
    }
