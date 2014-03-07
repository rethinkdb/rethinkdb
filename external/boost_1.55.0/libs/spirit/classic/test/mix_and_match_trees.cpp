//
//  Copyright (c) 2006 Joao Abecasis
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

////////////////////////////////////////////////////////////////////////////////
//
//  As reported by Jascha Wetzel, in
//  http://article.gmane.org/gmane.comp.parsers.spirit.general/9013, the
//  directives gen_ast_node_d and gen_pt_node_d were not working for lack of
//  appropriate conversion constructors in the underlying tree match policies.
//
////////////////////////////////////////////////////////////////////////////////

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_parse_tree.hpp>

using namespace BOOST_SPIRIT_CLASSIC_NS;

struct my_grammar : grammar<my_grammar>
{
    template <class Scanner>
    struct definition
    {
        typedef
            scanner<
                typename Scanner::iterator_t,
                scanner_policies<
                    typename Scanner::iteration_policy_t,
                    ast_match_policy<
                        typename Scanner::match_policy_t::iterator_t,
                        typename Scanner::match_policy_t::factory_t
                    >,
                    typename Scanner::action_policy_t
                >
            > ast_scanner;

        typedef
            scanner<
                typename Scanner::iterator_t,
                scanner_policies<
                    typename Scanner::iteration_policy_t,
                    pt_match_policy<
                        typename Scanner::match_policy_t::iterator_t,
                        typename Scanner::match_policy_t::factory_t
                    >,
                    typename Scanner::action_policy_t
                >
            > pt_scanner;

        typedef rule<ast_scanner> ast_rule;
        typedef rule<pt_scanner> pt_rule;
        typedef rule<Scanner> rule_;

        definition(my_grammar const & /* self */)
        {
            start_ = gen_ast_node_d[ ast_rule_ ];
            start_ = gen_pt_node_d[ pt_rule_ ];
        }

        rule_ const & start() const
        {
            return start_;
        }

        rule_ start_;
        ast_rule ast_rule_;
        pt_rule pt_rule_;
    };
};

int main()
{
    const char * begin, * end;

    pt_parse(begin, end, my_grammar());
    ast_parse(begin, end, my_grammar());
}
