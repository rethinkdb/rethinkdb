//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The purpose of this example is to show how a simple custom generator
//  directive can be written. We develop a custom generator allowing to wrap 
//  the generated output after each 5th column.
//
//  For more information see: http://boost-spirit.com/home/?page_id=659 

#include <boost/spirit/include/karma_generate_attr.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_numeric.hpp>

#include <string>
#include "simple_columns_directive.hpp"

namespace karma = boost::spirit::karma;

int main()
{
    using custom_generator::columns;

    std::vector<int> v;
    for (int i = 0; i < 17; ++i)
        v.push_back(i);

    std::string generated;
    std::back_insert_iterator<std::string> sink(generated);

    bool result = karma::generate_delimited(
        sink, columns[*karma::int_], karma::space, v);
    if (result) 
    {
        std::cout << "-------------------------------- \n";
        std::cout << "Generation succeeded\n";
        std::cout << "generated output: " << "\n" << generated << "\n";
        std::cout << "-------------------------------- \n";
    }
    else 
    {
        std::cout << "-------------------------------- \n";
        std::cout << "Generation failed\n";
        std::cout << "-------------------------------- \n";
    }
    return 0;
}
