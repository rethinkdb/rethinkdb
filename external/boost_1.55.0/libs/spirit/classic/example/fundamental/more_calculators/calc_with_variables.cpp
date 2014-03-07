/*=============================================================================
    Copyright (c) 2001-2003 Dan Nuffer
    Copyright (c) 2001-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  Full calculator example with variables
//  [ JDG 9/18/2002 ]
//
///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_symbols.hpp>
#include <iostream>
#include <stack>
#include <functional>
#include <string>

using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
//
//  Semantic actions
//
///////////////////////////////////////////////////////////////////////////////
struct push_num
{
    push_num(stack<double>& eval_)
    : eval(eval_) {}

    void operator()(double n) const
    {
        eval.push(n);
        cout << "push\t" << n << endl;
    }

    stack<double>& eval;
};

template <typename op>
struct do_op
{
    do_op(op const& the_op, stack<double>& eval_)
    : m_op(the_op), eval(eval_) {}

    void operator()(char const*, char const*) const
    {
        double rhs = eval.top();
        eval.pop();
        double lhs = eval.top();
        eval.pop();

        cout << "popped " << lhs << " and " << rhs << " from the stack. ";
        cout << "pushing " << m_op(lhs, rhs) << " onto the stack.\n";
        eval.push(m_op(lhs, rhs));
    }

    op m_op;
    stack<double>& eval;
};

template <class op>
do_op<op>
make_op(op const& the_op, stack<double>& eval)
{
    return do_op<op>(the_op, eval);
}

struct do_negate
{
    do_negate(stack<double>& eval_)
    : eval(eval_) {}

    void operator()(char const*, char const*) const
    {
        double lhs = eval.top();
        eval.pop();

        cout << "popped " << lhs << " from the stack. ";
        cout << "pushing " << -lhs << " onto the stack.\n";
        eval.push(-lhs);
    }

    stack<double>& eval;
};

struct get_var
{
    get_var(stack<double>& eval_)
    : eval(eval_) {}

    void operator()(double n) const
    {
        eval.push(n);
        cout << "push\t" << n << endl;
    }

    stack<double>& eval;
};

struct set_var
{
    set_var(double*& var_)
    : var(var_) {}

    void operator()(double& n) const
    {
        var = &n;
    }

    double*& var;
};

struct redecl_var
{
    void operator()(double& /*n*/) const
    {
        cout << "Warning. You are attempting to re-declare a var.\n";
    }
};

struct do_assign
{
    do_assign(double*& var_, stack<double>& eval_)
    : var(var_), eval(eval_) {}

    void operator()(char const*, char const*) const
    {
        if (var != 0)
        {
            *var = eval.top();
            cout << "assigning\n";
        }
    }

    double*& var;
    stack<double>&  eval;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Our calculator grammar
//
///////////////////////////////////////////////////////////////////////////////
struct calculator : public grammar<calculator>
{
    calculator(stack<double>& eval_)
    : eval(eval_) {}

    template <typename ScannerT>
    struct definition
    {
        definition(calculator const& self)
        {
            factor =
                    real_p[push_num(self.eval)]
                |   vars[get_var(self.eval)]
                |   '(' >> expression >> ')'
                |   ('-' >> factor)[do_negate(self.eval)]
                ;

            term =
                factor
                >> *(   ('*' >> factor)[make_op(multiplies<double>(), self.eval)]
                    |   ('/' >> factor)[make_op(divides<double>(), self.eval)]
                    )
                    ;

            expression =
                term
                >> *(  ('+' >> term)[make_op(plus<double>(), self.eval)]
                    |   ('-' >> term)[make_op(minus<double>(), self.eval)]
                    )
                    ;

            assignment =
                vars[set_var(self.var)]
                >> '=' >> expression[do_assign(self.var, self.eval)]
                ;

            var_decl =
                lexeme_d
                [
                    ((alpha_p >> *(alnum_p | '_'))
                        - vars[redecl_var()])[vars.add]
                ]
                ;

            declaration =
                lexeme_d["var" >> space_p] >> var_decl >> *(',' >> var_decl)
                ;

            statement =
                declaration | assignment | '?' >> expression
                ;
        }

        symbols<double>     vars;
        rule<ScannerT>      statement, declaration, var_decl,
                            assignment, expression, term, factor;

        rule<ScannerT> const&
        start() const { return statement; }
    };

    mutable double* var;
    stack<double>&  eval;
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
    cout << "\t\tThe calculator with variables...\n\n";
    cout << "/////////////////////////////////////////////////////////\n\n";
    cout << "Type a statement...or [q or Q] to quit\n\n";
    cout << "Variables may be declared:\t\tExample: var i, j, k\n";
    cout << "Assigning to a variable:\t\tExample: i = 10 * j\n";
    cout << "To evaluate an expression:\t\tExample: ? i * 3.33E-3\n\n";

    stack<double>   eval;
    calculator      calc(eval); //  Our parser

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


