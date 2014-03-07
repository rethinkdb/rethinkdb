// Boost.Geometry (aka GGL, Generic Geometry Library)
// doxygen_xml2qbk Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <iostream>
#include <string>

#include "fruit.hpp"


int main()
{
    fruit::apple<> a("my apple");
    eat(a);

    return 0;
}

