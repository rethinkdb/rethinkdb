//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The main purpose of this example is to show how a single container type can
//  be formatted using different output grammars. 

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/karma_stream.hpp>

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib> 

using namespace boost::spirit;
using namespace boost::spirit::ascii;

///////////////////////////////////////////////////////////////////////////////
int main()
{
    ///////////////////////////////////////////////////////////////////////////
    // vector
    std::vector<int> v (8);
    std::generate(v.begin(), v.end(), std::rand); // randomly fill the vector

    std::cout << "Output 8 integers from a std::vector<int>..." << std::endl;

    // output the container as a sequence without any separation
    std::cout << "...without any separation" << std::endl;
    std::cout << 
        karma::format(
            *int_,                                // format description
            v                                     // data
        ) << std::endl << std::endl;

    // output the container as a space separated sequence
    std::cout << "...as space delimited list" << std::endl;
    std::cout << 
        karma::format_delimited(
            *int_,                                // format description
            space,                                // delimiter
            v                                     // data
        ) << std::endl << std::endl;

    std::cout << 
        karma::format_delimited(
            '[' << *int_ << ']',                  // format description
            space,                                // delimiter
            v                                     // data
        ) << std::endl << std::endl;

    // output the container as a comma separated list
    std::cout << "...as comma separated list" << std::endl;
    std::cout << 
        karma::format(
            int_ % ", ",                          // format description
            v                                     // data
        ) << std::endl << std::endl;

    std::cout << 
        karma::format(
            '[' << (int_ % ", ") << ']',          // format description
            v                                     // data
        ) << std::endl << std::endl;

    // output the container as a comma separated list of double's
    std::cout << "...as comma separated list of doubles" << std::endl;
    std::cout << 
        karma::format(
            double_ % ", ",                       // format description
            v                                     // data
        ) << std::endl << std::endl;

    // output the container as a comma separated list of items enclosed in '()'
    std::cout << "..as list of ints enclosed in '()'" << std::endl;
    std::cout << 
        karma::format(
            ('(' << int_ << ')') % ", ",          // format description
            v                                     // data
        ) << std::endl << std::endl;

    std::cout << 
        karma::format(
            '[' << (  
                ('(' << int_ << ')') % ", "
             )  << ']',                           // format description
            v                                     // data
        ) << std::endl << std::endl;
        
    // output the container as a HTML list
    std::cout << "...as HTML bullet list" << std::endl;
    std::cout << 
        karma::format_delimited(
            "<ol>" << 
                // no delimiting within verbatim
                *verbatim["  <li>" << int_ << "</li>"]
            << "</ol>",                           // format description
            '\n',                                 // delimiter
            v                                     // data
        ) << std::endl;

    // output the container as right aligned column
    std::cout << "...right aligned in a column" << std::endl;
    std::cout << 
        karma::format_delimited(
           *verbatim[
                "|" << right_align[int_] << "|"
            ],                                    // format description
            '\n',                                 // delimiter
            v                                     // data
        ) << std::endl;

    std::cout << std::endl;
    return 0;
}

