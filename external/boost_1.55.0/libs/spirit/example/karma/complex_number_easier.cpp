//   Copyright (c) 2001-2010 Hartmut Kaiser
//   Copyright (c) 2001-2010 Joel de Guzman
// 
//   Distributed under the Boost Software License, Version 1.0. (See accompanying
//   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

///////////////////////////////////////////////////////////////////////////////
//
//  A complex number micro generator - take 2. Look'ma no semantic actions!
//
//  [ HK July 26, 2009 ]  spirit2
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
    //  Our complex number parser/compiler (that's just a copy of the complex 
    //  number example from Qi (see examples/qi/complex_number.cpp)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    bool parse_complex(Iterator first, Iterator last, std::complex<double>& c)
    {
        using boost::spirit::qi::double_;
        using boost::spirit::qi::_1;
        using boost::spirit::qi::phrase_parse;
        using boost::spirit::ascii::space;
        using boost::phoenix::ref;

        double rN = 0.0;
        double iN = 0.0;
        bool r = phrase_parse(first, last,
            (
                    '(' >> double_[ref(rN) = _1]
                        >> -(',' >> double_[ref(iN) = _1]) >> ')'
                |   double_[ref(rN) = _1]
            ),
            space);

        if (!r || first != last) // fail if we did not get a full match
            return false;
        c = std::complex<double>(rN, iN);
        return r;
    }

    ///////////////////////////////////////////////////////////////////////////
    //  Our complex number generator
    ///////////////////////////////////////////////////////////////////////////
    //[tutorial_karma_complex_number_easier
    template <typename OutputIterator>
    bool generate_complex(OutputIterator sink, std::complex<double> const& c)
    {
        using boost::spirit::karma::double_;
        using boost::spirit::karma::omit;
        using boost::spirit::karma::generate;

        return generate(sink,

            //  Begin grammar
            (
               !double_(0.0) << '(' << double_ << ", " << double_ << ')'
            |   omit[double_] << double_
            ),
            //  End grammar

            c.imag(), c.real(), c.imag()     //  Data to output
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
    std::cout << "\t\tA complex number micro generator for Spirit...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";

    std::cout << "Give me a complex number of the form r or (r) or (r,i) \n";
    std::cout << "Type [q or Q] to quit\n\n";

    std::string str;
    while (getline(std::cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        std::complex<double> c;
        if (client::parse_complex(str.begin(), str.end(), c))
        {
            std::cout << "-------------------------\n";

            std::string generated;
            std::back_insert_iterator<std::string> sink(generated);
            if (!client::generate_complex(sink, c))
            {
                std::cout << "-------------------------\n";
                std::cout << "Generating failed\n";
                std::cout << "-------------------------\n";
            }
            else
            {
                std::cout << "-------------------------\n";
                std::cout << "Generated: " << generated << "\n";
                std::cout << "-------------------------\n";
            }
        }
        else
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed\n";
            std::cout << "-------------------------\n";
        }
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}


