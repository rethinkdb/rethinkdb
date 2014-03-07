/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
//[porting_guide_qi_includes
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <iostream>
#include <string>
#include <algorithm>
//]

//[porting_guide_qi_namespace
using namespace boost::spirit;
//]

//[porting_guide_qi_grammar
template <typename Iterator>
struct roman : qi::grammar<Iterator, unsigned()>
{
    roman() : roman::base_type(first)
    {
        hundreds.add
            ("C"  , 100)("CC"  , 200)("CCC"  , 300)("CD" , 400)("D" , 500)
            ("DC" , 600)("DCC" , 700)("DCCC" , 800)("CM" , 900) ;

        tens.add
            ("X"  , 10)("XX"  , 20)("XXX"  , 30)("XL" , 40)("L" , 50)
            ("LX" , 60)("LXX" , 70)("LXXX" , 80)("XC" , 90) ;

        ones.add
            ("I"  , 1)("II"  , 2)("III"  , 3)("IV" , 4)("V" , 5)
            ("VI" , 6)("VII" , 7)("VIII" , 8)("IX" , 9) ;

        // qi::_val refers to the attribute of the rule on the left hand side 
        first = eps          [qi::_val = 0] 
            >>  (  +lit('M') [qi::_val += 1000]
                ||  hundreds [qi::_val += qi::_1]
                ||  tens     [qi::_val += qi::_1]
                ||  ones     [qi::_val += qi::_1]
                ) ;
    }

    qi::rule<Iterator, unsigned()> first;
    qi::symbols<char, unsigned> hundreds;
    qi::symbols<char, unsigned> tens;
    qi::symbols<char, unsigned> ones;
};
//]

int main()
{
    {
        //[porting_guide_qi_parse
        std::string input("1,1");
        std::string::iterator it = input.begin();
        bool result = qi::parse(it, input.end(), qi::int_);

        if (result) 
            std::cout << "successful match!\n";

        if (it == input.end()) 
            std::cout << "full match!\n";
        else
            std::cout << "stopped at: " << std::string(it, input.end()) << "\n";

        // seldomly needed: use std::distance to calculate the length of the match
        std::cout << "matched length: " << std::distance(input.begin(), it) << "\n";
        //]
    }

    {
        //[porting_guide_qi_phrase_parse
        std::string input(" 1, 1");
        std::string::iterator it = input.begin();
        bool result = qi::phrase_parse(it, input.end(), qi::int_, ascii::space);

        if (result) 
            std::cout << "successful match!\n";

        if (it == input.end()) 
            std::cout << "full match!\n";
        else
            std::cout << "stopped at: " << std::string(it, input.end()) << "\n";

        // seldomly needed: use std::distance to calculate the length of the match
        std::cout << "matched length: " << std::distance(input.begin(), it) << "\n";
        //]
    }

    {
        //[porting_guide_qi_use_grammar
        std::string input("MMIX");        // MMIX == 2009
        std::string::iterator it = input.begin();
        unsigned value = 0;
        roman<std::string::iterator> r;
        if (qi::parse(it, input.end(), r, value)) 
            std::cout << "successfully matched: " << value << "\n";
        //]
    }
    return 0;
}

