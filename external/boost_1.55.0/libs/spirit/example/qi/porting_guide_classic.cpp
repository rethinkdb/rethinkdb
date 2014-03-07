/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
//[porting_guide_classic_includes
#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/phoenix1.hpp>
#include <iostream>
#include <string>
//]

//[porting_guide_classic_namespace
using namespace boost::spirit::classic;
//]

//[porting_guide_classic_grammar
struct roman : public grammar<roman>
{
    template <typename ScannerT>
    struct definition
    {
        definition(roman const& self)
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

            first = eps_p         [phoenix::var(self.r) = phoenix::val(0)]
                >>  (  +ch_p('M') [phoenix::var(self.r) += phoenix::val(1000)]
                    ||  hundreds  [phoenix::var(self.r) += phoenix::_1]
                    ||  tens      [phoenix::var(self.r) += phoenix::_1]
                    ||  ones      [phoenix::var(self.r) += phoenix::_1] 
                    ) ;
        }

        rule<ScannerT> first;
        symbols<unsigned> hundreds;
        symbols<unsigned> tens;
        symbols<unsigned> ones;

        rule<ScannerT> const& start() const { return first; }
    };

    roman(unsigned& r_) : r(r_) {}
    unsigned& r;
};
//]

int main()
{
    {
        //[porting_guide_classic_parse
        std::string input("1,1");
        parse_info<std::string::iterator> pi = parse(input.begin(), input.end(), int_p);

        if (pi.hit) 
            std::cout << "successful match!\n";

        if (pi.full) 
            std::cout << "full match!\n";
        else
            std::cout << "stopped at: " << std::string(pi.stop, input.end()) << "\n";

        std::cout << "matched length: " << pi.length << "\n";
        //]
    }

    {
        //[porting_guide_classic_phrase_parse
        std::string input(" 1, 1");
        parse_info<std::string::iterator> pi = parse(input.begin(), input.end(), int_p, space_p);

        if (pi.hit) 
            std::cout << "successful match!\n";

        if (pi.full) 
            std::cout << "full match!\n";
        else
            std::cout << "stopped at: " << std::string(pi.stop, input.end()) << "\n";

        std::cout << "matched length: " << pi.length << "\n";
        //]
    }

    {
        //[porting_guide_classic_use_grammar
        std::string input("MMIX");        // MMIX == 2009
        unsigned value = 0;
        roman r(value);
        parse_info<std::string::iterator> pi = parse(input.begin(), input.end(), r);
        if (pi.hit) 
            std::cout << "successfully matched: " << value << "\n";
        //]
    }
    return 0;
}

