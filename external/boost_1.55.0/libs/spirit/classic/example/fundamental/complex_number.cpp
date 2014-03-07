/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  A complex number micro parser (using subrules)
//
//  [ JDG 5/10/2002 ]
//
///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/include/classic_core.hpp>
#include <iostream>
#include <complex>
#include <string>

///////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
//
//  Our complex number micro parser
//
///////////////////////////////////////////////////////////////////////////////
bool
parse_complex(char const* str, complex<double>& c)
{
    double rN = 0.0;
    double iN = 0.0;

    subrule<0> first;
    subrule<1> r;
    subrule<2> i;

    if (parse(str,

        //  Begin grammar
        (
            first = '(' >> r >> !(',' >> i) >> ')' | r,
            r = real_p[assign(rN)],
            i = real_p[assign(iN)]
        )
        ,
        //  End grammar

        space_p).full)
    {
        c = complex<double>(rN, iN);
        return true;
    }
    else
    {
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  Main program
//
////////////////////////////////////////////////////////////////////////////
int
main()
{
    cout << "/////////////////////////////////////////////////////////\n\n";
    cout << "\t\tA complex number micro parser for Spirit...\n\n";
    cout << "/////////////////////////////////////////////////////////\n\n";

    cout << "Give me a complex number of the form r or (r) or (r,i) \n";
    cout << "Type [q or Q] to quit\n\n";

    string str;
    while (getline(cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        complex<double> c;
        if (parse_complex(str.c_str(), c))
        {
            cout << "-------------------------\n";
            cout << "Parsing succeeded\n";
            cout << str << " Parses OK: " << c << endl;
            cout << "-------------------------\n";
        }
        else
        {
            cout << "-------------------------\n";
            cout << "Parsing failed\n";
            cout << "-------------------------\n";
        }
    }

    cout << "Bye... :-) \n\n";
    return 0;
}


