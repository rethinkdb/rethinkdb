/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2001-2010 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  A Calculator example demonstrating generation of AST from which we generate 
//  a simple byte code representation being interpreted by a similar virtual 
//  machine.
//
//  [ JDG April 28, 2008 ]
//  [ HK May 05, 2008 ]
//
///////////////////////////////////////////////////////////////////////////////
#include <boost/config/warning_disable.hpp>

#include <iostream>
#include <vector>
#include <string>

#include "calc2_ast_vm.hpp"

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

using namespace boost::spirit;
using namespace boost::spirit::ascii;

///////////////////////////////////////////////////////////////////////////////
//  Our calculator parser grammar
///////////////////////////////////////////////////////////////////////////////
template <typename Iterator>
struct calculator 
  : qi::grammar<Iterator, expression_ast(), space_type>
{
    calculator() : calculator::base_type(expression)
    {
        expression =
            term                            [_val = _1]
            >> *(   ('+' >> term            [_val += _1])
                |   ('-' >> term            [_val -= _1])
                )
            ;

        term =
            factor                          [_val = _1]
            >> *(   ('*' >> factor          [_val *= _1])
                |   ('/' >> factor          [_val /= _1])
                )
            ;

        factor =
            uint_                           [_val = _1]
            |   '(' >> expression           [_val = _1] >> ')'
            |   ('-' >> factor              [_val = neg(_1)])
            |   ('+' >> factor              [_val = pos(_1)])
            ;
    }

    qi::rule<Iterator, expression_ast(), space_type> expression, term, factor;
};

///////////////////////////////////////////////////////////////////////////////
//  The Virtual Machine
///////////////////////////////////////////////////////////////////////////////
class vmachine
{
public:
    union element {
        int code;
        char bytes[sizeof(int)];
    };

    vmachine(unsigned stackSize = 4096)
      : stack(stackSize)
      , stack_ptr(stack.begin())
    {
    }

    int top() const { return stack_ptr[-1]; };
    void execute(std::vector<element> const& code);

private:
    std::vector<int> stack;
    std::vector<int>::iterator stack_ptr;
};

void vmachine::execute(std::vector<element> const& code)
{
    std::vector<element>::const_iterator pc = code.begin();
    stack_ptr = stack.begin();

    while ((*pc).code && pc != code.end())
    {
        switch ((*pc++).code)
        {
            case op_neg:
                stack_ptr[-1] = -stack_ptr[-1];
                break;

            case op_add:
                --stack_ptr;
                stack_ptr[-1] += stack_ptr[0];
                break;

            case op_sub:
                --stack_ptr;
                stack_ptr[-1] -= stack_ptr[0];
                break;

            case op_mul:
                --stack_ptr;
                stack_ptr[-1] *= stack_ptr[0];
                break;

            case op_div:
                --stack_ptr;
                stack_ptr[-1] /= stack_ptr[0];
                break;

            case op_int:
                *stack_ptr++ = (*pc++).code;
                break;
        }
    }
}

// We need to tell fusion about our binary_op and unary_op structs
// to make them a first-class fusion citizen
//
// Note: we register the members exactly in the same sequence as we need them 
//       in the grammar
BOOST_FUSION_ADAPT_STRUCT(
    binary_op,
    (expression_ast, left)
    (expression_ast, right)
    (int, op)
)

BOOST_FUSION_ADAPT_STRUCT(
    unary_op,
    (expression_ast, right)
    (int, op)
)

///////////////////////////////////////////////////////////////////////////////
//  Our AST grammar for the generator, this just dumps the AST as a expression
///////////////////////////////////////////////////////////////////////////////
template <typename OuputIterator, typename Delimiter>
struct generate_byte_code
  : karma::grammar<OuputIterator, expression_ast(), Delimiter>
{
    generate_byte_code() : generate_byte_code::base_type(ast_node)
    {
        ast_node %= int_node | binary_node | unary_node;
        int_node %= dword(op_int) << dword;
        binary_node %= ast_node << ast_node << byte_;
        unary_node %= ast_node << byte_;
    }

    karma::rule<OuputIterator, expression_ast(), Delimiter> ast_node;
    karma::rule<OuputIterator, int(), Delimiter> int_node;
    karma::rule<OuputIterator, binary_op(), Delimiter> binary_node;
    karma::rule<OuputIterator, unary_op(), Delimiter> unary_node;
};

///////////////////////////////////////////////////////////////////////////////
// helper function helping to deduce the delimiter type
template <typename Delimiter>
bool generate_vm_code(expression_ast const& ast, 
    std::vector<vmachine::element>& code, Delimiter const& d)
{
    // Our generator grammar definitions
    typedef char* output_iterator_type;
    typedef generate_byte_code<output_iterator_type, Delimiter> generate_byte_code;

    char* outbuffer = (*code.begin()).bytes;
    generate_byte_code gen_vm;
    return karma::generate_delimited(outbuffer, gen_vm, d, ast);
}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Compile simple expressions to bytecode...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Type an expression...or [q or Q] to quit\n\n";

    //  Our parser grammar definitions
    typedef std::string::const_iterator iterator_type;
    typedef calculator<iterator_type> calculator;

    calculator calc; 

    std::string str;
    while (std::getline(std::cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        expression_ast ast;
        std::string::const_iterator iter = str.begin();
        std::string::const_iterator end = str.end();
        bool r = qi::phrase_parse(iter, end, calc, space, ast);

        if (r && iter == end)
        {
            // we assume a vm code size of 4096 is sufficient
            std::vector<vmachine::element> code (4096);
            r = generate_vm_code(ast, code, pad(4));

            if (r)
            {
                vmachine vm;
                vm.execute(code);
                std::cout << "\nresult = " << vm.top() << std::endl;
                std::cout << "-------------------------\n";
            }
            else
            {
                std::cout << "-------------------------\n";
                std::cout << "Generating failed\n";
                std::cout << "-------------------------\n";
            }
        }
        else
        {
            std::string rest(iter, end);
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed\n";
            std::cout << "stopped at: \": " << rest << "\"\n";
            std::cout << "-------------------------\n";
        }
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}


