/*=============================================================================
    Copyright (c) 2002-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  Demonstrates use of boost::bind and spirit
//  This is discussed in the "Functional" chapter in the Spirit User's Guide.
//
//  [ JDG 9/29/2002 ]
//
///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/include/classic_core.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <vector>
#include <string>

///////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;
using namespace boost;

///////////////////////////////////////////////////////////////////////////////
//
//  Our comma separated list parser
//
///////////////////////////////////////////////////////////////////////////////
class list_parser
{
public:

    typedef list_parser self_t;

    bool
    parse(char const* str)
    {
        return BOOST_SPIRIT_CLASSIC_NS::parse(str,

            //  Begin grammar
            (
                real_p
                [
                    bind(&self_t::add, this, _1)
                ]

                >> *(   ','
                        >>  real_p
                            [
                                bind(&self_t::add, this, _1)
                            ]
                    )
            )
            ,
            //  End grammar

            space_p).full;
    }

    void
    add(double n)
    {
        v.push_back(n);
    }

    void
    print() const
    {
        for (vector<double>::size_type i = 0; i < v.size(); ++i)
            cout << i << ": " << v[i] << endl;
    }

    vector<double> v;
};

////////////////////////////////////////////////////////////////////////////
//
//  Main program
//
////////////////////////////////////////////////////////////////////////////
int
main()
{
    cout << "/////////////////////////////////////////////////////////\n\n";
    cout << "\tA comma separated list parser for Spirit...\n";
    cout << "\tDemonstrates use of boost::bind and spirit\n";
    cout << "/////////////////////////////////////////////////////////\n\n";

    cout << "Give me a comma separated list of numbers.\n";
    cout << "The numbers will be inserted in a vector of numbers\n";
    cout << "Type [q or Q] to quit\n\n";

    string str;
    while (getline(cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        list_parser lp;
        if (lp.parse(str.c_str()))
        {
            cout << "-------------------------\n";
            cout << "Parsing succeeded\n";
            cout << str << " Parses OK: " << endl;

            lp.print();

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


