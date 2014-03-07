/*=============================================================================
    Copyright (c) 2001-2003 Daniel Nuffer
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
// JDG 4-16-03 Modified from ast_calc.cpp as a test

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_tree_to_xml.hpp>
#include <boost/detail/workaround.hpp>

#include <iostream>
#include <stack>
#include <functional>
#include <string>
#include <boost/detail/lightweight_test.hpp>

using namespace BOOST_SPIRIT_CLASSIC_NS;

////////////////////////////////////////////////////////////////////////////
//
//  Our calculator grammar
//
////////////////////////////////////////////////////////////////////////////
struct calculator : public grammar<calculator>
{
    static const int integerID = 1;
    static const int factorID = 2;
    static const int termID = 3;
    static const int expressionID = 4;

    template <typename ScannerT>
    struct definition
    {
        definition(calculator const& /*self*/)
        {
            //  Start grammar definition
            integer     =   leaf_node_d[real_p]; // we're not really using a real
                                                 // but just for compile checking
                                                 // the AST tree match code...
            factor      =   integer
                        |   inner_node_d[ch_p('(') >> expression >> ch_p(')')]
                        |   (root_node_d[ch_p('-')] >> factor);

            term        =   factor >>
                            *(  (root_node_d[ch_p('*')] >> factor)
                              | (root_node_d[ch_p('/')] >> factor)
                            );

            expression  =   term >>
                            *(  (root_node_d[ch_p('+')] >> term)
                              | (root_node_d[ch_p('-')] >> term)
                            );
            //  End grammar definition
        }

        rule<ScannerT, parser_context<>, parser_tag<expressionID> >   expression;
        rule<ScannerT, parser_context<>, parser_tag<termID> >         term;
        rule<ScannerT, parser_context<>, parser_tag<factorID> >       factor;
        rule<ScannerT, parser_context<>, parser_tag<integerID> >      integer;

        rule<ScannerT, parser_context<>, parser_tag<expressionID> > const&
        start() const { return expression; }
    };
};

////////////////////////////////////////////////////////////////////////////
//
//  Our calculator grammar, but with dynamically assigned rule ID's
//
////////////////////////////////////////////////////////////////////////////
struct dyn_calculator : public grammar<dyn_calculator>
{
    static const int integerID = 1;
    static const int factorID = 2;
    static const int termID = 3;
    static const int expressionID = 4;

    template <typename ScannerT>
    struct definition
    {
        definition(dyn_calculator const& /*self*/)
        {
            expression.set_id(expressionID);
            term.set_id(termID);
            factor.set_id(factorID);
            integer.set_id(integerID);
            
            //  Start grammar definition
            integer     =   leaf_node_d[real_p]; // we're not really using a real
                                                 // but just for compile checking
                                                 // the AST tree match code...
            factor      =   integer
                        |   inner_node_d[ch_p('(') >> expression >> ch_p(')')]
                        |   (root_node_d[ch_p('-')] >> factor);

            term        =   factor >>
                            *(  (root_node_d[ch_p('*')] >> factor)
                              | (root_node_d[ch_p('/')] >> factor)
                            );

            expression  =   term >>
                            *(  (root_node_d[ch_p('+')] >> term)
                              | (root_node_d[ch_p('-')] >> term)
                            );
            //  End grammar definition
        }

        rule<ScannerT, parser_context<>, dynamic_parser_tag>  expression;
        rule<ScannerT, parser_context<>, dynamic_parser_tag>  term;
        rule<ScannerT, parser_context<>, dynamic_parser_tag>  factor;
        rule<ScannerT, parser_context<>, dynamic_parser_tag>  integer;

        rule<ScannerT, parser_context<>, dynamic_parser_tag> const&
        start() const { return expression; }
    };
};

////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

typedef char const*                         iterator_t;
typedef tree_match<iterator_t>              parse_tree_match_t;
typedef parse_tree_match_t::tree_iterator   iter_t;

////////////////////////////////////////////////////////////////////////////
long evaluate(parse_tree_match_t hit);
long eval_expression(iter_t const& i);

long evaluate(tree_parse_info<> info)
{
    return eval_expression(info.trees.begin());
}

long eval_expression(iter_t const& i)
{
    switch (i->value.id().to_long())
    {
        case calculator::integerID:
        {
            BOOST_TEST(i->children.size() == 0);
            // extract integer (not always delimited by '\0')
#if BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3003))
            // std::string(iter,iter) constructor has a bug in MWCW 8.3: 
            //  in some situations, the null terminator won't be added
            //  and c_str() will return bogus data. Conservatively, I 
            //  activate this workaround up to version 8.3.
            std::vector<char> value(i->value.begin(), i->value.end());
            value.push_back('\0');
            return strtol(&value[0], 0, 10);
#else
            string integer(i->value.begin(), i->value.end());
            return strtol(integer.c_str(), 0, 10);
#endif
        }

        case calculator::factorID:
        {
            // factor can only be unary minus
            BOOST_TEST(*i->value.begin() == '-');
            return - eval_expression(i->children.begin());
        }

        case calculator::termID:
        {
            if (*i->value.begin() == '*')
            {
                BOOST_TEST(i->children.size() == 2);
                return eval_expression(i->children.begin()) *
                    eval_expression(i->children.begin()+1);
            }
            else if (*i->value.begin() == '/')
            {
                BOOST_TEST(i->children.size() == 2);
                return eval_expression(i->children.begin()) /
                    eval_expression(i->children.begin()+1);
            }
            else
                BOOST_TEST(0);
        }

        case calculator::expressionID:
        {
            if (*i->value.begin() == '+')
            {
                BOOST_TEST(i->children.size() == 2);
                return eval_expression(i->children.begin()) +
                    eval_expression(i->children.begin()+1);
            }
            else if (*i->value.begin() == '-')
            {
                BOOST_TEST(i->children.size() == 2);
                return eval_expression(i->children.begin()) -
                    eval_expression(i->children.begin()+1);
            }
            else
                BOOST_TEST(0);
        }

        default:
            BOOST_TEST(0); // error
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////
int
parse(char const* str)
{
    calculator calc;
    tree_parse_info<> info = ast_parse(str, calc, space_p);

    if (info.full)
        return evaluate(info);
    else
        return -1;
}

int
parse_dyn(char const* str)
{
    dyn_calculator calc;
    tree_parse_info<> info = ast_parse(str, calc, space_p);

    if (info.full)
        return evaluate(info);
    else
        return -1;
}

int
main()
{
// test the calculator with statically assigned rule ID's
    BOOST_TEST(parse("12345") == 12345);
    BOOST_TEST(parse("-12345") == -12345);
    BOOST_TEST(parse("1 + 2") == 1 + 2);
    BOOST_TEST(parse("1 * 2") == 1 * 2);
    BOOST_TEST(parse("1/2 + 3/4") == 1/2 + 3/4);
    BOOST_TEST(parse("1 + 2 + 3 + 4") == 1 + 2 + 3 + 4);
    BOOST_TEST(parse("1 * 2 * 3 * 4") == 1 * 2 * 3 * 4);
    BOOST_TEST(parse("(1 + 2) * (3 + 4)") == (1 + 2) * (3 + 4));
    BOOST_TEST(parse("(-1 + 2) * (3 + -4)") == (-1 + 2) * (3 + -4));
    BOOST_TEST(parse("1 + ((6 * 200) - 20) / 6") == 1 + ((6 * 200) - 20) / 6);
    BOOST_TEST(parse("(1 + (2 + (3 + (4 + 5))))") == (1 + (2 + (3 + (4 + 5)))));
    BOOST_TEST(parse("1 + 2 + 3 + 4 + 5") == 1 + 2 + 3 + 4 + 5);
    BOOST_TEST(parse("(12 * 22) + (36 + -4 + 5)") == (12 * 22) + (36 + -4 + 5));
    BOOST_TEST(parse("(12 * 22) / (5 - 10 + 15)") == (12 * 22) / (5 - 10 + 15));
    BOOST_TEST(parse("12 * 6 * 15 + 5 - 25") == 12 * 6 * 15 + 5 - 25);

// test the calculator with dynamically assigned rule ID's
    BOOST_TEST(parse_dyn("12345") == 12345);
    BOOST_TEST(parse_dyn("-12345") == -12345);
    BOOST_TEST(parse_dyn("1 + 2") == 1 + 2);
    BOOST_TEST(parse_dyn("1 * 2") == 1 * 2);
    BOOST_TEST(parse_dyn("1/2 + 3/4") == 1/2 + 3/4);
    BOOST_TEST(parse_dyn("1 + 2 + 3 + 4") == 1 + 2 + 3 + 4);
    BOOST_TEST(parse_dyn("1 * 2 * 3 * 4") == 1 * 2 * 3 * 4);
    BOOST_TEST(parse_dyn("(1 + 2) * (3 + 4)") == (1 + 2) * (3 + 4));
    BOOST_TEST(parse_dyn("(-1 + 2) * (3 + -4)") == (-1 + 2) * (3 + -4));
    BOOST_TEST(parse_dyn("1 + ((6 * 200) - 20) / 6") == 1 + ((6 * 200) - 20) / 6);
    BOOST_TEST(parse_dyn("(1 + (2 + (3 + (4 + 5))))") == (1 + (2 + (3 + (4 + 5)))));
    BOOST_TEST(parse_dyn("1 + 2 + 3 + 4 + 5") == 1 + 2 + 3 + 4 + 5);
    BOOST_TEST(parse_dyn("(12 * 22) + (36 + -4 + 5)") == (12 * 22) + (36 + -4 + 5));
    BOOST_TEST(parse_dyn("(12 * 22) / (5 - 10 + 15)") == (12 * 22) / (5 - 10 + 15));
    BOOST_TEST(parse_dyn("12 * 6 * 15 + 5 - 25") == 12 * 6 * 15 + 5 - 25);

    return boost::report_errors();    
}


