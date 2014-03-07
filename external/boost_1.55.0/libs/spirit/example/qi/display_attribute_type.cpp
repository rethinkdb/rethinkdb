//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This example implements a simple utility allowing to print the attribute 
//  type as it is exposed by an arbitrary Qi parser expression. Just insert 
//  your expression below, compile and run this example to see what Qi is 
//  seeing!

#include "display_attribute_type.hpp"

namespace qi = boost::spirit::qi;

int main()
{
    tools::display_attribute_of_parser(
        std::cerr,                // put the required output stream here
        qi::int_ >> qi::double_   // put your parser expression here
    );
    return 0;
}
