/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
////////////////////////////////////////////////////////////////////////////
//
//  The calculator using a simple virtual machine and compiler.
//
//  Ported to v1.5 from the original v1.0 code by JDG
//  [ JDG 9/18/2002 ]
//
////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/include/classic_core.hpp>
#include <iostream>
#include <vector>
#include <string>

////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
//
//  The VMachine
//
///////////////////////////////////////////////////////////////////////////////
enum ByteCodes
{
    OP_NEG,     //  negate the top stack entry
    OP_ADD,     //  add top two stack entries
    OP_SUB,     //  subtract top two stack entries
    OP_MUL,     //  multiply top two stack entries
    OP_DIV,     //  divide top two stack entries
    OP_INT,     //  push constant integer into the stack
    OP_RET      //  return from the interpreter
};

class vmachine
{
public:
                vmachine(unsigned stackSize = 1024)
                :   stack(new int[stackSize]),
                    stackPtr(stack) {}
                ~vmachine() { delete [] stack; }
    int         top() const { return stackPtr[-1]; };
    void        execute(int code[]);

private:

    int*        stack;
    int*        stackPtr;
};

void
vmachine::execute(int code[])
{
    int const*  pc = code;
    bool        running = true;
    stackPtr = stack;

    while (running)
    {
        switch (*pc++)
        {
            case OP_NEG:
                stackPtr[-1] = -stackPtr[-1];
                break;

            case OP_ADD:
                stackPtr--;
                stackPtr[-1] += stackPtr[0];
                break;

            case OP_SUB:
                stackPtr--;
                stackPtr[-1] -= stackPtr[0];
                break;

            case OP_MUL:
                stackPtr--;
                stackPtr[-1] *= stackPtr[0];
                break;

            case OP_DIV:
                stackPtr--;
                stackPtr[-1] /= stackPtr[0];
                break;

            case OP_INT:
                // Check stack overflow here!
                *stackPtr++ = *pc++;
                break;

            case OP_RET:
                running = false;
                break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  The Compiler
//
///////////////////////////////////////////////////////////////////////////////
struct push_int
{
    push_int(vector<int>& code_)
    : code(code_) {}

    void operator()(char const* str, char const* /*end*/) const
    {
        using namespace std;
        int n = strtol(str, 0, 10);
        code.push_back(OP_INT);
        code.push_back(n);
        cout << "push\t" << int(n) << endl;
    }

    vector<int>& code;
};

struct push_op
{
    push_op(int op_, vector<int>& code_)
    : op(op_), code(code_) {}

    void operator()(char const*, char const*) const
    {
        code.push_back(op);

        switch (op) {

            case OP_NEG:
                cout << "neg\n";
                break;

            case OP_ADD:
                cout << "add\n";
                break;

            case OP_SUB:
                cout << "sub\n";
                break;

            case OP_MUL:
                cout << "mul\n";
                break;

            case OP_DIV:
                cout << "div\n";
                break;
        }
    }

    int op;
    vector<int>& code;
};

template <typename GrammarT>
static bool
compile(GrammarT const& calc, char const* expr)
{
    cout << "\n/////////////////////////////////////////////////////////\n\n";

    parse_info<char const*>
        result = parse(expr, calc, space_p);

    if (result.full)
    {
        cout << "\t\t" << expr << " Parses OK\n\n\n";
        calc.code.push_back(OP_RET);
        return true;
    }
    else
    {
        cout << "\t\t" << expr << " Fails parsing\n";
        cout << "\t\t";
        for (int i = 0; i < (result.stop - expr); i++)
            cout << " ";
        cout << "^--Here\n\n\n";
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  Our calculator grammar
//
////////////////////////////////////////////////////////////////////////////
struct calculator : public grammar<calculator>
{
    calculator(vector<int>& code_)
    : code(code_) {}

    template <typename ScannerT>
    struct definition
    {
        definition(calculator const& self)
        {
            integer =
                lexeme_d[ (+digit_p)[push_int(self.code)] ]
                ;

            factor =
                    integer
                |   '(' >> expression >> ')'
                |   ('-' >> factor)[push_op(OP_NEG, self.code)]
                |   ('+' >> factor)
                ;

            term =
                factor
                >> *(   ('*' >> factor)[push_op(OP_MUL, self.code)]
                    |   ('/' >> factor)[push_op(OP_DIV, self.code)]
                    )
                    ;

            expression =
                term
                >> *(   ('+' >> term)[push_op(OP_ADD, self.code)]
                    |   ('-' >> term)[push_op(OP_SUB, self.code)]
                    )
                    ;
        }

        rule<ScannerT> expression, term, factor, integer;

        rule<ScannerT> const&
        start() const { return expression; }
    };

    vector<int>& code;
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
    cout << "\t\tA simple virtual machine...\n\n";
    cout << "/////////////////////////////////////////////////////////\n\n";
    cout << "Type an expression...or [q or Q] to quit\n\n";

    vmachine    mach;       //  Our virtual machine
    vector<int> code;       //  Our VM code
    calculator  calc(code); //  Our parser

    string str;
    while (getline(cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        code.clear();
        if (compile(calc, str.c_str()))
        {
            mach.execute(&*code.begin());
            cout << "\n\nresult = " << mach.top() << "\n\n";
        }
    }

    cout << "Bye... :-) \n\n";
    return 0;
}


