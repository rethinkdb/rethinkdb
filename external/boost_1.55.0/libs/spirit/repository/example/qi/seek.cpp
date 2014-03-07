/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2011 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/

//  [ Jamboree Oct 27, 2011 ]       new example.


#include <cstdlib>
#include <iostream>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/repository/include/qi_seek.hpp>


int main(int argc, char *argv[])
{
    //[reference_qi_seek_namespace
    namespace qi = boost::spirit::qi;
    namespace repo = boost::spirit::repository;
    //]
    
    typedef std::string::const_iterator iterator;

    //[reference_qi_seek_vars
    std::string str("/*C-style comment*/");
    iterator it = str.begin();
    iterator end = str.end();
    //]

    //[reference_qi_seek_parse
    if (qi::parse(it, end, "/*" >> repo::qi::seek["*/"]))
    {
        std::cout << "-------------------------------- \n";
        std::cout << "Parsing succeeded.\n";
        std::cout << "---------------------------------\n";
    }
    else
    {
        std::cout << "-------------------------------- \n";
        std::cout << "Unterminated /* comment.\n";
        std::cout << "-------------------------------- \n";
    }//]

    return EXIT_SUCCESS;
}
