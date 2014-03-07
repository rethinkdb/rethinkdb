//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The purpose of this example is to demonstrate different use cases for the
//  confix generator.

#include <iostream>
#include <string>
#include <vector>

//[karma_confix_includes
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/repository/include/karma_confix.hpp>
//]

//[karma_confix_namespace
using namespace boost::spirit;
using namespace boost::spirit::ascii;
using boost::spirit::repository::confix;
//]

int main()
{
//[karma_confix_cpp_comment
    // C++ comment
    std::cout << 
        karma::format_delimited(
            confix("//", eol)[string],            // format description
            space,                                // delimiter
            "This is a comment"                   // data
        ) << std::endl;
//]

//[karma_confix_c_comment
    // C comment
    std::cout << 
        karma::format_delimited(
            confix("/*", "*/")[string],           // format description
            space,                                // delimiter
            "This is a comment"                   // data
        ) << std::endl;
//]

//[karma_confix_function
    // Generate a function call with an arbitrary parameter list
    std::vector<std::string> parameters;
    parameters.push_back("par1");
    parameters.push_back("par2");
    parameters.push_back("par3");

    std::cout << 
        karma::format(
            string << confix('(', ')')[string % ','],   // format description
            "func",                                     // function name
            parameters                                  // parameter names
        ) << std::endl;
//]

    return 0;
}

