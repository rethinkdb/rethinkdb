/*=============================================================================
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  Full calculator example
//  [ demonstrating phoenix and subrules ]
//
//  [ Hartmut Kaiser 10/8/2002 ]
//
///////////////////////////////////////////////////////////////////////////////

//#define BOOST_SPIRIT_DEBUG        // define this for debug output

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_attribute.hpp>
#include <iostream>
#include <string>

///////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;
using namespace phoenix;

///////////////////////////////////////////////////////////////////////////////
//
//  Our calculator grammar using phoenix to do the semantics and subrule's
//  as it's working horses
//
//  Note:   The top rule propagates the expression result (value) upwards
//          to the calculator grammar self.val closure member which is
//          then visible outside the grammar (i.e. since self.val is the
//          member1 of the closure, it becomes the attribute passed by
//          the calculator to an attached semantic action. See the
//          driver code that uses the calculator below).
//
///////////////////////////////////////////////////////////////////////////////
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
            top = (
                expression =
                    term[self.val = arg1]
                    >> *(   ('+' >> term[self.val += arg1])
                        |   ('-' >> term[self.val -= arg1])
                        )
                ,

                term =
                    factor[term.val = arg1]
                    >> *(   ('*' >> factor[term.val *= arg1])
                        |   ('/' >> factor[term.val /= arg1])
                        )
                ,

                factor
                    =    ureal_p[factor.val = arg1]
                    |   '(' >> expression[factor.val = arg1] >> ')'
                    |   ('-' >> factor[factor.val = -arg1])
                    |   ('+' >> factor[factor.val = arg1])
            );

            BOOST_SPIRIT_DEBUG_NODE(top);
            BOOST_SPIRIT_DEBUG_NODE(expression);
            BOOST_SPIRIT_DEBUG_NODE(term);
            BOOST_SPIRIT_DEBUG_NODE(factor);
        }

        subrule<0, calc_closure::context_t>  expression;
        subrule<1, calc_closure::context_t>  term;
        subrule<2, calc_closure::context_t>  factor;

        rule<ScannerT> top;

        rule<ScannerT> const&
        start() const { return top; }
    };
};

///////////////////////////////////////////////////////////////////////////////
//
//  Main program
//
///////////////////////////////////////////////////////////////////////////////
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


