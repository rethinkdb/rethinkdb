/*=============================================================================
    Copyright (c) 2002-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
////////////////////////////////////////////////////////////////////////////
//
//  Plain calculator example demostrating the grammar and semantic actions.
//  This is discussed in the "Grammar" and "Semantic Actions" chapters in
//  the Spirit User's Guide.
//
//  [ JDG 5/10/2002 ]
//
////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/include/classic_core.hpp>
#include <iostream>
#include <string>

////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

////////////////////////////////////////////////////////////////////////////
//
//  Semantic actions
//
////////////////////////////////////////////////////////////////////////////
namespace
{
    void    do_int(char const* str, char const* end)
    {
        string  s(str, end);
        cout << "PUSH(" << s << ')' << endl;
    }

    void    do_add(char const*, char const*)    { cout << "ADD\n"; }
    void    do_subt(char const*, char const*)   { cout << "SUBTRACT\n"; }
    void    do_mult(char const*, char const*)   { cout << "MULTIPLY\n"; }
    void    do_div(char const*, char const*)    { cout << "DIVIDE\n"; }
    void    do_neg(char const*, char const*)    { cout << "NEGATE\n"; }
}

////////////////////////////////////////////////////////////////////////////
//
//  Our calculator grammar
//
////////////////////////////////////////////////////////////////////////////
struct calculator : public grammar<calculator>
{
    template <typename ScannerT>
    struct definition
    {
        definition(calculator const& /*self*/)
        {
            expression
                =   term
                    >> *(   ('+' >> term)[&do_add]
                        |   ('-' >> term)[&do_subt]
                        )
                ;

            term
                =   factor
                    >> *(   ('*' >> factor)[&do_mult]
                        |   ('/' >> factor)[&do_div]
                        )
                ;

            factor
                =   lexeme_d[(+digit_p)[&do_int]]
                |   '(' >> expression >> ')'
                |   ('-' >> factor)[&do_neg]
                |   ('+' >> factor)
                ;
        }

        rule<ScannerT> expression, term, factor;

        rule<ScannerT> const&
        start() const { return expression; }
    };
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
    cout << "\t\tExpression parser...\n\n";
    cout << "/////////////////////////////////////////////////////////\n\n";
    cout << "Type an expression...or [q or Q] to quit\n\n";

    calculator calc;    //  Our parser

    string str;
    while (getline(cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        parse_info<> info = parse(str.c_str(), calc, space_p);

        if (info.full)
        {
            cout << "-------------------------\n";
            cout << "Parsing succeeded\n";
            cout << "-------------------------\n";
        }
        else
        {
            cout << "-------------------------\n";
            cout << "Parsing failed\n";
            cout << "stopped at: \": " << info.stop << "\"\n";
            cout << "-------------------------\n";
        }
    }

    cout << "Bye... :-) \n\n";
    return 0;
}


