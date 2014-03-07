/*=============================================================================
    Copyright (c) 2002-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
////////////////////////////////////////////////////////////////////////////
//
//  Full calculator example demonstrating Phoenix
//  This is discussed in the "Closures" chapter in the Spirit User's Guide.
//
//  [ JDG 6/29/2002 ]
//
////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_attribute.hpp>
#include <iostream>
#include <string>

////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;
using namespace phoenix;

////////////////////////////////////////////////////////////////////////////
//
//  Our calculator grammar using phoenix to do the semantics
//
//  Note:   The top rule propagates the expression result (value) upwards
//          to the calculator grammar self.val closure member which is
//          then visible outside the grammar (i.e. since self.val is the
//          member1 of the closure, it becomes the attribute passed by
//          the calculator to an attached semantic action. See the
//          driver code that uses the calculator below).
//
////////////////////////////////////////////////////////////////////////////
struct calc_closure : BOOST_SPIRIT_CLASSIC_NS::closure<calc_closure, double>
{
    member1 val;
};

struct calculator : public grammar<calculator, calc_closure::context_t>
{
    template <typename ScannerT>
    struct definition
    {
        definition(calculator const& self)
        {
            top = expression[self.val = arg1];

            expression
                =   term[expression.val = arg1]
                    >> *(   ('+' >> term[expression.val += arg1])
                        |   ('-' >> term[expression.val -= arg1])
                        )
                ;

            term
                =   factor[term.val = arg1]
                    >> *(   ('*' >> factor[term.val *= arg1])
                        |   ('/' >> factor[term.val /= arg1])
                        )
                ;

            factor
                =   ureal_p[factor.val = arg1]
                |   '(' >> expression[factor.val = arg1] >> ')'
                |   ('-' >> factor[factor.val = -arg1])
                |   ('+' >> factor[factor.val = arg1])
                ;
        }

        typedef rule<ScannerT, calc_closure::context_t> rule_t;
        rule_t expression, term, factor;
        rule<ScannerT> top;

        rule<ScannerT> const&
        start() const { return top; }
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
    cout << "\t\tExpression parser using Phoenix...\n\n";
    cout << "/////////////////////////////////////////////////////////\n\n";
    cout << "Type an expression...or [q or Q] to quit\n\n";

    calculator calc;    //  Our parser

    string str;
    while (getline(cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        double n = 0;
        parse_info<> info = parse(str.c_str(), calc[var(n) = arg1], space_p);

        //  calc[var(n) = arg1] invokes the calculator and extracts
        //  the result of the computation. See calculator grammar
        //  note above.

        if (info.full)
        {
            cout << "-------------------------\n";
            cout << "Parsing succeeded\n";
            cout << "result = " << n << endl;
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


