/*=============================================================================
    Copyright (c) 2002-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  A parser for a real number parser that parses thousands separated numbers
//  with at most two decimal places and no exponent. This is discussed in the
//  "Numerics" chapter in the Spirit User's Guide.
//
//  [ JDG 12/16/2003 ]
//
///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
#include <iostream>
#include <string>

///////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

template <typename T>
struct ts_real_parser_policies : public ureal_parser_policies<T>
{
    //  These policies can be used to parse thousand separated
    //  numbers with at most 2 decimal digits after the decimal
    //  point. e.g. 123,456,789.01

    typedef uint_parser<int, 10, 1, 2>  uint2_t;
    typedef uint_parser<T, 10, 1, -1>   uint_parser_t;
    typedef int_parser<int, 10, 1, -1>  int_parser_t;

    //////////////////////////////////  2 decimal places Max
    template <typename ScannerT>
    static typename parser_result<uint2_t, ScannerT>::type
    parse_frac_n(ScannerT& scan)
    { return uint2_t().parse(scan); }

    //////////////////////////////////  No exponent
    template <typename ScannerT>
    static typename parser_result<chlit<>, ScannerT>::type
    parse_exp(ScannerT& scan)
    { return scan.no_match(); }

    //////////////////////////////////  No exponent
    template <typename ScannerT>
    static typename parser_result<int_parser_t, ScannerT>::type
    parse_exp_n(ScannerT& scan)
    { return scan.no_match(); }

    //////////////////////////////////  Thousands separated numbers
    template <typename ScannerT>
    static typename parser_result<uint_parser_t, ScannerT>::type
    parse_n(ScannerT& scan)
    {
        typedef typename parser_result<uint_parser_t, ScannerT>::type RT;
        static uint_parser<unsigned, 10, 1, 3> uint3_p;
        static uint_parser<unsigned, 10, 3, 3> uint3_3_p;
        if (RT hit = uint3_p.parse(scan))
        {
            T n;
            typedef typename ScannerT::iterator_t iterator_t;
            iterator_t save = scan.first;
            while (match<> next = (',' >> uint3_3_p[assign_a(n)]).parse(scan))
            {
                hit.value((hit.value() * 1000) + n);
                scan.concat_match(hit, next);
                save = scan.first;
            }
            scan.first = save;
            return hit;

            // Note: On erroneous input such as "123,45", the result should
            // be a partial match "123". 'save' is used to makes sure that
            // the scanner position is placed at the last *valid* parse
            // position.
        }
        return scan.no_match();
    }
};

real_parser<double, ts_real_parser_policies<double> > const
    ts_real_p = real_parser<double, ts_real_parser_policies<double> >();

////////////////////////////////////////////////////////////////////////////
//
//  Main program
//
////////////////////////////////////////////////////////////////////////////
int
main()
{
    cout << "/////////////////////////////////////////////////////////\n\n";
    cout << "\t\tA real number parser that parses thousands separated\n";
    cout << "\t\tnumbers with at most two decimal places and no exponent...\n\n";
    cout << "/////////////////////////////////////////////////////////\n\n";

    cout << "Give me a number.\n";
    cout << "Type [q or Q] to quit\n\n";

    string str;
    double n;
    while (getline(cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        if (parse(str.c_str(), ts_real_p[assign_a(n)]).full)
        {
            cout << "-------------------------\n";
            cout << "Parsing succeeded\n";
            cout << str << " Parses OK: " << endl;
            cout << "n=" << n << endl;
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


