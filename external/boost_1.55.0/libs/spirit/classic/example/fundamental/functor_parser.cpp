/*=============================================================================
    Copyright (c) 2002-2003 Joel de Guzman
    Copyright (c) 2002 Juan Carlos Arevalo-Baeza
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_functor_parser.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
#include <iostream>
#include <vector>
#include <string>

///////////////////////////////////////////////////////////////////////////////
//
//  Demonstrates the functor_parser. This is discussed in the
//  "Functor Parser" chapter in the Spirit User's Guide.
//
///////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
//
//  Our parser functor
//
///////////////////////////////////////////////////////////////////////////////
struct number_parser
{
    typedef int result_t;
    template <typename ScannerT>
    int
    operator()(ScannerT const& scan, result_t& result) const
    {
        if (scan.at_end())
            return -1;

        char ch = *scan;
        if (ch < '0' || ch > '9')
            return -1;

        result = 0;
        int len = 0;

        do
        {
            result = result*10 + int(ch - '0');
            ++len;
            ++scan;
        } while (!scan.at_end() && (ch = *scan, ch >= '0' && ch <= '9'));

        return len;
    }
};

functor_parser<number_parser> number_parser_p;

///////////////////////////////////////////////////////////////////////////////
//
//  Our number parser functions
//
///////////////////////////////////////////////////////////////////////////////
bool
parse_number(char const* str, int& n)
{
    return parse(str, lexeme_d[number_parser_p[assign_a(n)]], space_p).full;
}

bool
parse_numbers(char const* str, std::vector<int>& n)
{
    return
        parse(
            str,
            lexeme_d[number_parser_p[push_back_a(n)]]
                >> *(',' >> lexeme_d[number_parser_p[push_back_a(n)]]),
            space_p
        ).full;
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
    cout << "\t\tA number parser implemented as a functor for Spirit...\n\n";
    cout << "/////////////////////////////////////////////////////////\n\n";

    cout << "Give me an integer number command\n";
    cout << "Commands:\n";
    cout << "  A <num> --> parses a single number\n";
    cout << "  B <num>, <num>, ... --> parses a series of numbers ";
    cout << "separated by commas\n";
    cout << "  Q --> quit\n\n";

    string str;
    while (getline(cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        else if (str[0] == 'a' || str[0] == 'A')
        {
            int n;
            if (parse_number(str.c_str()+1, n))
            {
                cout << "-------------------------\n";
                cout << "Parsing succeeded\n";
                cout << str << " Parses OK: " << n << endl;
                cout << "-------------------------\n";
            }
            else
            {
                cout << "-------------------------\n";
                cout << "Parsing failed\n";
                cout << "-------------------------\n";
            }
        }

        else if (str[0] == 'b' || str[0] == 'B')
        {
            std::vector<int> n;
            if (parse_numbers(str.c_str()+1, n))
            {
                cout << "-------------------------\n";
                cout << "Parsing succeeded\n";
                int size = n.size();
                cout << str << " Parses OK: " << size << " number(s): " << n[0];
                for (int i = 1; i < size; ++i) {
                    cout << ", " << n[i];
                }
                cout << endl;
                cout << "-------------------------\n";
            }
            else
            {
                cout << "-------------------------\n";
                cout << "Parsing failed\n";
                cout << "-------------------------\n";
            }
        }

        else
        {
            cout << "-------------------------\n";
            cout << "Unrecognized command!!";
            cout << "-------------------------\n";
        }
    }

    cout << "Bye... :-) \n\n";
    return 0;
}
