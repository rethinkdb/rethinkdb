/*=============================================================================
    Copyright (c) 2001-2003 Hartmut Kaiser
    Copyright (c) 2002-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  This sample shows, how to use Phoenix for implementing a
//  simple (RPN style) calculator [ demonstrating phoenix ]
//
//  [ HKaiser 2001 ]
//  [ JDG 6/29/2002 ]
//
///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_attribute.hpp>
#include <boost/spirit/include/phoenix1_functions.hpp>
#include <iostream>
#include <string>

///////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;
using namespace phoenix;

///////////////////////////////////////////////////////////////////////////////
//
//  Our RPN calculator grammar using phoenix to do the semantics
//  The class 'RPNCalculator' implements a polish reverse notation
//  calculator which is equivalent to the following YACC description.
//
//  exp:
//        NUM           { $$ = $1;           }
//      | exp exp '+'   { $$ = $1 + $2;      }
//      | exp exp '-'   { $$ = $1 - $2;      }
//      | exp exp '*'   { $$ = $1 * $2;      }
//      | exp exp '/'   { $$ = $1 / $2;      }
//      | exp exp '^'   { $$ = pow ($1, $2); }  /* Exponentiation */
//      | exp 'n'       { $$ = -$1;          }  /* Unary minus */
//      ;
//
//  The different notation results from the requirement of LL parsers not to
//  allow left recursion in their grammar (would lead to endless recursion).
//  Therefore the left recursion in the YACC script before is transformated
//  into iteration. To some, this is less intuitive, but once you get used
//  to it, it's very easy to follow.
//
//  Note:   The top rule propagates the expression result (value) upwards
//          to the calculator grammar self.val closure member which is
//          then visible outside the grammar (i.e. since self.val is the
//          member1 of the closure, it becomes the attribute passed by
//          the calculator to an attached semantic action. See the
//          driver code that uses the calculator below).
//
///////////////////////////////////////////////////////////////////////////////
struct pow_
{
    template <typename X, typename Y>
    struct result { typedef X type; };

    template <typename X, typename Y>
    X operator()(X x, Y y) const
    {
        using namespace std;
        return pow(x, y);
    }
};

//  Notice how power(x, y) is lazily implemented using Phoenix function.
function<pow_> power;

struct calc_closure : BOOST_SPIRIT_CLASSIC_NS::closure<calc_closure, double, double>
{
    member1 x;
    member2 y;
};

struct calculator : public grammar<calculator, calc_closure::context_t>
{
    template <typename ScannerT>
    struct definition {

        definition(calculator const& self)
        {
            top = expr                      [self.x = arg1];
            expr =
                real_p                      [expr.x = arg1]
                >> *(
                        expr                [expr.y = arg1]
                        >>  (
                                ch_p('+')   [expr.x += expr.y]
                            |   ch_p('-')   [expr.x -= expr.y]
                            |   ch_p('*')   [expr.x *= expr.y]
                            |   ch_p('/')   [expr.x /= expr.y]
                            |   ch_p('^')   [expr.x = power(expr.x, expr.y)]
                            )
                    |   ch_p('n')           [expr.x = -expr.x]
                    )
                ;
        }

        typedef rule<ScannerT, calc_closure::context_t> rule_t;
        rule_t expr;
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


