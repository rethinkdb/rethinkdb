//   Copyright (c) 2001-2010 Hartmut Kaiser
// 
//   Distributed under the Boost Software License, Version 1.0. (See accompanying
//   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

///////////////////////////////////////////////////////////////////////////////
//
//  A character classification example
//
//  [ HK August 12, 2009 ]  spirit2
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <iostream>
#include <string>
#include <complex>

namespace client
{
    ///////////////////////////////////////////////////////////////////////////
    //  Our character classification generator
    ///////////////////////////////////////////////////////////////////////////
    //[tutorial_karma_complex_number
    template <typename OutputIterator>
    bool classify_character(OutputIterator sink, char c)
    {
        using boost::spirit::ascii::char_;
        using boost::spirit::ascii::digit;
        using boost::spirit::ascii::xdigit;
        using boost::spirit::ascii::alpha;
        using boost::spirit::ascii::punct;
        using boost::spirit::ascii::space;
        using boost::spirit::ascii::cntrl;
        using boost::spirit::karma::omit;
        using boost::spirit::karma::generate;

        if (!boost::spirit::char_encoding::ascii::isascii_(c))
            return false;

        return generate(sink,
            //  Begin grammar
            (
                "The character '" << char_ << "' is "
                <<  (   &digit  << "a digit"
                    |   &xdigit << "a xdigit"
                    |   &alpha  << "a alpha"
                    |   &punct  << "a punct"
                    |   &space  << "a space"
                    |   &cntrl  << "a cntrl"
                    |   "of unknown type"
                    )
            ),
            //  End grammar
            c, c
        );
    }
    //]
}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "\t\tA character classification micro generator for Spirit...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";

    std::cout << "Give me a character to classify\n";
    std::cout << "Type [q or Q] to quit\n\n";

    std::string str;
    while (getline(std::cin, str))
    {
        if (str.empty())
            break;

        std::string generated;
        std::back_insert_iterator<std::string> sink(generated);
        if (!client::classify_character(sink, str[0]))
        {
            std::cout << "-------------------------\n";
            std::cout << "Generating failed\n";
            std::cout << "-------------------------\n";
        }
        else
        {
            std::cout << generated << "\n";
        }
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}


