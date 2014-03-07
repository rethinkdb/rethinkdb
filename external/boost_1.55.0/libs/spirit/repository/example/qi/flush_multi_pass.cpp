//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The purpose of this example is to demonstrate a simple use case for the
//  flush_multi_pass parser.

#include <iostream>
#include <fstream>
#include <string>

//[qi_flush_multi_pass_includes
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/repository/include/qi_flush_multi_pass.hpp>
//]

//[qi_flush_multi_pass_namespace
namespace spirit = boost::spirit;
using boost::spirit::repository::flush_multi_pass;
//]

namespace client
{
    //[qi_flush_multi_pass_clear_buffer
    template <typename Iterator, typename Skipper>
    struct preprocessor : spirit::qi::grammar<Iterator, Skipper>
    {
        // This is a simplified preprocessor grammar recognizing
        //
        //   #define MACRONAME something
        //   #undef MACRONAME
        //
        // Its sole purpose is to show an example how to use the 
        // flush_multi_pass parser. At the end of each line no backtracking can
        // occur anymore so that it's safe to clear all internal buffers in the 
        // multi_pass.
        preprocessor() : preprocessor::base_type(file)
        {
            using spirit::ascii::char_;
            using spirit::qi::eol;
            using spirit::qi::lit;

            file = 
                *line
                ;

            line =  ( command |  *(char_ - eol) ) 
                >>  eol 
                >>  flush_multi_pass
                ;

            command =
                    "#define" >> *lit(' ') >> *(char_ - ' ') >> *lit(' ') >> *(char_ - eol)
                |   "#undef"  >> *lit(' ') >> *(char_ - eol)
                ;
        }

        spirit::qi::rule<Iterator, Skipper> file, line, command;
    };
    //]
}

template <typename Iterator, typename Skipper>
bool parse(Iterator& first, Iterator end, Skipper const& skipper)
{
    client::preprocessor<Iterator, Skipper> g;
    return boost::spirit::qi::phrase_parse(first, end, g, skipper);
}

int main()
{
    namespace spirit = boost::spirit;
    using spirit::ascii::char_;
    using spirit::qi::eol;

    std::ifstream in("flush_multi_pass.txt");    // we get our input from this file
    if (!in.is_open()) {
        std::cout << "Could not open input file: 'flush_multi_pass.txt'" << std::endl;
        return -1;
    }

    typedef std::istreambuf_iterator<char> base_iterator_type;
    spirit::multi_pass<base_iterator_type> first = 
        spirit::make_default_multi_pass(base_iterator_type(in));
    spirit::multi_pass<base_iterator_type> end = 
        spirit::make_default_multi_pass(base_iterator_type());

    bool result = parse(first, end, '#' >> *(char_ - eol) >> eol);
    if (!result) {
        std::cout << "Failed parsing input file!" << std::endl;
        return -2;
    }

    std::cout << "Successfully parsed input file!" << std::endl;
    return 0;
}

