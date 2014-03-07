/*=============================================================================
    Copyright (c) 2001-2003 Daniel Nuffer
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_ast.hpp>
#include <boost/assert.hpp>

#include <iostream>
#include <stack>
#include <functional>
#include <string>

// This example shows how to use an AST and tree_iter_node instead of
// tree_val_node
////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

typedef char const*         iterator_t;
typedef tree_match<iterator_t, node_iter_data_factory<> >
    parse_tree_match_t;

typedef parse_tree_match_t::tree_iterator iter_t;

typedef ast_match_policy<iterator_t, node_iter_data_factory<> > match_policy_t;
typedef scanner<iterator_t, scanner_policies<iter_policy_t, match_policy_t> > scanner_t;
typedef rule<scanner_t> rule_t;


//  grammar rules
rule_t expression, term, factor, integer;

////////////////////////////////////////////////////////////////////////////
long evaluate(parse_tree_match_t hit);
long eval_expression(iter_t const& i);
long eval_term(iter_t const& i);
long eval_factor(iter_t const& i);
long eval_integer(iter_t const& i);

long evaluate(parse_tree_match_t hit)
{
    return eval_expression(hit.trees.begin());
}

long eval_expression(iter_t const& i)
{
    cout << "In eval_expression. i->value = " <<
        string(i->value.begin(), i->value.end()) <<
        " i->children.size() = " << i->children.size() << endl;

    cout << "ID: " << i->value.id().to_long() << endl;

    if (i->value.id() == integer.id())
    {
        BOOST_ASSERT(i->children.size() == 0);
        return strtol(i->value.begin(), 0, 10);
    }
    else if (i->value.id() == factor.id())
    {
        // factor can only be unary minus
        BOOST_ASSERT(*i->value.begin() == '-');
        return - eval_expression(i->children.begin());
    }
    else if (i->value.id() == term.id())
    {
        if (*i->value.begin() == '*')
        {
            BOOST_ASSERT(i->children.size() == 2);
            return eval_expression(i->children.begin()) *
                eval_expression(i->children.begin()+1);
        }
        else if (*i->value.begin() == '/')
        {
            BOOST_ASSERT(i->children.size() == 2);
            return eval_expression(i->children.begin()) /
                eval_expression(i->children.begin()+1);
        }
        else
            BOOST_ASSERT(0);
    }
    else if (i->value.id() == expression.id())
    {
        if (*i->value.begin() == '+')
        {
            BOOST_ASSERT(i->children.size() == 2);
            return eval_expression(i->children.begin()) +
                eval_expression(i->children.begin()+1);
        }
        else if (*i->value.begin() == '-')
        {
            BOOST_ASSERT(i->children.size() == 2);
            return eval_expression(i->children.begin()) -
                eval_expression(i->children.begin()+1);
        }
        else
            BOOST_ASSERT(0);
    }
    else
        BOOST_ASSERT(0); // error

   return 0;
}

////////////////////////////////////////////////////////////////////////////
int
main()
{
    BOOST_SPIRIT_DEBUG_RULE(integer);
    BOOST_SPIRIT_DEBUG_RULE(factor);
    BOOST_SPIRIT_DEBUG_RULE(term);
    BOOST_SPIRIT_DEBUG_RULE(expression);
    //  Start grammar definition
    integer     =   leaf_node_d[ lexeme_d[ (!ch_p('-') >> +digit_p) ] ];
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


    cout << "/////////////////////////////////////////////////////////\n\n";
    cout << "\t\tThe simplest working calculator...\n\n";
    cout << "/////////////////////////////////////////////////////////\n\n";
    cout << "Type an expression...or [q or Q] to quit\n\n";

    string str;
    while (getline(cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        const char* str_begin = str.c_str();
        const char* str_end = str.c_str();
        while (*str_end)
            ++str_end;

        scanner_t scan(str_begin, str_end);

        parse_tree_match_t hit = expression.parse(scan);


        if (hit && str_begin == str_end)
        {
#if defined(BOOST_SPIRIT_DUMP_PARSETREE_AS_XML)
            // dump parse tree as XML
            std::map<rule_id, std::string> rule_names;
            rule_names[&integer] = "integer";
            rule_names[&factor] = "factor";
            rule_names[&term] = "term";
            rule_names[&expression] = "expression";
            tree_to_xml(cout, hit.trees, str.c_str(), rule_names);
#endif

            // print the result
            cout << "parsing succeeded\n";
            cout << "result = " << evaluate(hit) << "\n\n";
        }
        else
        {
            cout << "parsing failed\n";
        }
    }

    cout << "Bye... :-) \n\n";
    return 0;
}


