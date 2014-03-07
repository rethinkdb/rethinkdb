/*=============================================================================
    Copyright (c) 2004 Chris Hoeppler
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// This test verifies, that reapeat_p et.al. work correctly while using AST's

# include <map>
# include <boost/detail/lightweight_test.hpp>
# include <iostream>
# include <string>

# include <boost/spirit/include/classic_core.hpp>
# include <boost/spirit/include/classic_loops.hpp>
# include <boost/spirit/include/classic_ast.hpp>
# include <boost/spirit/include/classic_tree_to_xml.hpp>

using namespace BOOST_SPIRIT_CLASSIC_NS;

static const int numID = 1;
static const int funcID = 2;
static const int expressionID = 3;

struct grammar_fail : public grammar<grammar_fail>
{
    template <typename ScannerT>
    struct definition
    {
        definition(grammar_fail const& /*self */)
        {
            num = leaf_node_d[real_p];
            func = root_node_d[ch_p('+') | '-']
                >> repeat_p(2)[expression];
            expression = func | num;
        }
        rule<ScannerT, parser_context<>, parser_tag<numID> > num;
        rule<ScannerT, parser_context<>, parser_tag<funcID> > func;
        typedef rule<ScannerT, parser_context<>, parser_tag<expressionID> > expr_t;
        expr_t expression;
        expr_t const& start() const { return expression; }
    };
};
struct grammar_success : public grammar<grammar_success>
{
    template <typename ScannerT>
    struct definition
    {
        definition(grammar_success const& /*self */)
        {
            num = leaf_node_d[real_p];
            func = root_node_d[ch_p('+') | '-']
                >> expression >> expression; // line differing from grammar_fail
            expression = func | num;
        }
        rule<ScannerT, parser_context<>, parser_tag<numID> > num;
        rule<ScannerT, parser_context<>, parser_tag<funcID> > func;
        typedef rule<ScannerT, parser_context<>, parser_tag<expressionID> > expr_t;
        expr_t expression;
        expr_t const& start() const { return expression; }
    };
};

int main() {

    std::map<parser_id, std::string> rule_names;
    rule_names[expressionID] = "expression";
    rule_names[funcID] = "func";
    rule_names[numID] = "num";

    std::string test("+ 1 - 2 3");

// case 1
    grammar_fail g_fail;
    tree_parse_info<> info1 = ast_parse(test.c_str(), g_fail, space_p);
    BOOST_TEST(info1.full);

    //std::cout << "...Case 1: Using repeat_p\n";
    //tree_to_xml(std::cerr, info1.trees, test, rule_names);
    
// case 2
    grammar_success g_success;
    tree_parse_info<> info2 = ast_parse(test.c_str(), g_success, space_p);
    BOOST_TEST(info2.full);

    //std::cout << "...Case 2: No repeat_p\n";
    //tree_to_xml(std::cerr, info2.trees, test, rule_names);
    // could be used for test case instead of printing the xml stuff:
    BOOST_TEST(info2.trees.begin()->children.size() == 2);
    BOOST_TEST(info1.trees.begin()->children.size() == 2);
    return boost::report_errors();
}
