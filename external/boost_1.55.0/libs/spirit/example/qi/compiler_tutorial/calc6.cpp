/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  Yet another calculator example! This time, we will compile to a simple
//  virtual machine. This is actually one of the very first Spirit example
//  circa 2000. Now, it's ported to Spirit2.
//
//  [ JDG Sometime 2000 ]       pre-boost
//  [ JDG September 18, 2002 ]  spirit1
//  [ JDG April 8, 2007 ]       spirit2
//  [ JDG February 18, 2011 ]   Pure attributes. No semantic actions.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Spirit v2.5 allows you to suppress automatic generation
// of predefined terminals to speed up complation. With
// BOOST_SPIRIT_NO_PREDEFINED_TERMINALS defined, you are
// responsible in creating instances of the terminals that
// you need (e.g. see qi::uint_type uint_ below).
#define BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Define this to enable debugging
//#define BOOST_SPIRIT_QI_DEBUG

///////////////////////////////////////////////////////////////////////////////
// Uncomment this if you want to enable debugging
//#define BOOST_SPIRIT_QI_DEBUG
///////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
# pragma warning(disable: 4345)
#endif

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/foreach.hpp>

#include <iostream>
#include <string>

namespace client { namespace ast
{
    ///////////////////////////////////////////////////////////////////////////
    //  The AST
    ///////////////////////////////////////////////////////////////////////////
    struct nil {};
    struct signed_;
    struct expression;

    typedef boost::variant<
            nil
          , unsigned int
          , boost::recursive_wrapper<signed_>
          , boost::recursive_wrapper<expression>
        >
    operand;

    struct signed_
    {
        char sign;
        operand operand_;
    };

    struct operation
    {
        char operator_;
        operand operand_;
    };

    struct expression
    {
        operand first;
        std::list<operation> rest;
    };

    // print function for debugging
    inline std::ostream& operator<<(std::ostream& out, nil) { out << "nil"; return out; }
}}

BOOST_FUSION_ADAPT_STRUCT(
    client::ast::signed_,
    (char, sign)
    (client::ast::operand, operand_)
)

BOOST_FUSION_ADAPT_STRUCT(
    client::ast::operation,
    (char, operator_)
    (client::ast::operand, operand_)
)

BOOST_FUSION_ADAPT_STRUCT(
    client::ast::expression,
    (client::ast::operand, first)
    (std::list<client::ast::operation>, rest)
)

namespace client
{
    ///////////////////////////////////////////////////////////////////////////
    //  The Virtual Machine
    ///////////////////////////////////////////////////////////////////////////
    enum byte_code
    {
        op_neg,     //  negate the top stack entry
        op_add,     //  add top two stack entries
        op_sub,     //  subtract top two stack entries
        op_mul,     //  multiply top two stack entries
        op_div,     //  divide top two stack entries
        op_int,     //  push constant integer into the stack
    };

    class vmachine
    {
    public:

        vmachine(unsigned stackSize = 4096)
          : stack(stackSize)
          , stack_ptr(stack.begin())
        {
        }

        int top() const { return stack_ptr[-1]; };
        void execute(std::vector<int> const& code);

    private:

        std::vector<int> stack;
        std::vector<int>::iterator stack_ptr;
    };

    void vmachine::execute(std::vector<int> const& code)
    {
        std::vector<int>::const_iterator pc = code.begin();
        stack_ptr = stack.begin();

        while (pc != code.end())
        {
            switch (*pc++)
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
                    *stack_ptr++ = *pc++;
                    break;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    //  The Compiler
    ///////////////////////////////////////////////////////////////////////////
    struct compiler
    {
        typedef void result_type;

        std::vector<int>& code;
        compiler(std::vector<int>& code)
          : code(code) {}

        void operator()(ast::nil) const { BOOST_ASSERT(0); }
        void operator()(unsigned int n) const
        {
            code.push_back(op_int);
            code.push_back(n);
        }

        void operator()(ast::operation const& x) const
        {
            boost::apply_visitor(*this, x.operand_);
            switch (x.operator_)
            {
                case '+': code.push_back(op_add); break;
                case '-': code.push_back(op_sub); break;
                case '*': code.push_back(op_mul); break;
                case '/': code.push_back(op_div); break;
                default: BOOST_ASSERT(0); break;
            }
        }

        void operator()(ast::signed_ const& x) const
        {
            boost::apply_visitor(*this, x.operand_);
            switch (x.sign)
            {
                case '-': code.push_back(op_neg); break;
                case '+': break;
                default: BOOST_ASSERT(0); break;
            }
        }

        void operator()(ast::expression const& x) const
        {
            boost::apply_visitor(*this, x.first);
            BOOST_FOREACH(ast::operation const& oper, x.rest)
            {
                (*this)(oper);
            }
        }
    };

    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;
    using boost::phoenix::function;

    ///////////////////////////////////////////////////////////////////////////////
    //  The error handler
    ///////////////////////////////////////////////////////////////////////////////
    struct error_handler_
    {
        template <typename, typename, typename>
        struct result { typedef void type; };

        template <typename Iterator>
        void operator()(
            qi::info const& what
          , Iterator err_pos, Iterator last) const
        {
            std::cout
                << "Error! Expecting "
                << what                         // what failed?
                << " here: \""
                << std::string(err_pos, last)   // iterators to error-pos, end
                << "\""
                << std::endl
            ;
        }
    };

    function<error_handler_> const error_handler = error_handler_();

    ///////////////////////////////////////////////////////////////////////////////
    //  The calculator grammar
    ///////////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    struct calculator : qi::grammar<Iterator, ast::expression(), ascii::space_type>
    {
        calculator() : calculator::base_type(expression)
        {
            qi::char_type char_;
            qi::uint_type uint_;
            qi::_2_type _2;
            qi::_3_type _3;
            qi::_4_type _4;

            using qi::on_error;
            using qi::fail;

            expression =
                term
                >> *(   (char_('+') > term)
                    |   (char_('-') > term)
                    )
                ;

            term =
                factor
                >> *(   (char_('*') > factor)
                    |   (char_('/') > factor)
                    )
                ;

            factor =
                    uint_
                |   '(' > expression > ')'
                |   (char_('-') > factor)
                |   (char_('+') > factor)
                ;

            // Debugging and error handling and reporting support.
            BOOST_SPIRIT_DEBUG_NODES(
                (expression)(term)(factor));

            // Error handling
            on_error<fail>(expression, error_handler(_4, _3, _2));
        }

        qi::rule<Iterator, ast::expression(), ascii::space_type> expression;
        qi::rule<Iterator, ast::expression(), ascii::space_type> term;
        qi::rule<Iterator, ast::operand(), ascii::space_type> factor;
    };
}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Expression parser...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Type an expression...or [q or Q] to quit\n\n";

    typedef std::string::const_iterator iterator_type;
    typedef client::calculator<iterator_type> calculator;
    typedef client::ast::expression ast_expression;
    typedef client::compiler compiler;

    std::string str;
    while (std::getline(std::cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        client::vmachine mach;      // Our virtual machine
        std::vector<int> code;      // Our VM code
        calculator calc;            // Our grammar
        ast_expression expression;  // Our program (AST)
        compiler compile(code);     // Compiles the program

        std::string::const_iterator iter = str.begin();
        std::string::const_iterator end = str.end();
        boost::spirit::ascii::space_type space;
        bool r = phrase_parse(iter, end, calc, space, expression);

        if (r && iter == end)
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded\n";
            compile(expression);
            mach.execute(code);
            std::cout << "\nResult: " << mach.top() << std::endl;
            std::cout << "-------------------------\n";
        }
        else
        {
            std::string rest(iter, end);
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed\n";
            std::cout << "-------------------------\n";
        }
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}


