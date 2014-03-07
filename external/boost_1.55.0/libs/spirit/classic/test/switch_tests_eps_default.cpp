/*=============================================================================
    Copyright (c) 2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <iostream>
#include <boost/detail/lightweight_test.hpp>

using namespace std;

#define BOOST_SPIRIT_SWITCH_CASE_LIMIT 6
#define BOOST_SPIRIT_SELECT_LIMIT 6
#define PHOENIX_LIMIT 6

//#define BOOST_SPIRIT_DEBUG
#include <boost/mpl/list.hpp>
#include <boost/mpl/for_each.hpp>

#include <boost/spirit/include/classic_primitives.hpp>
#include <boost/spirit/include/classic_numerics.hpp>
#include <boost/spirit/include/classic_actions.hpp>
#include <boost/spirit/include/classic_operators.hpp>
#include <boost/spirit/include/classic_rule.hpp>
#include <boost/spirit/include/classic_grammar.hpp>
#include <boost/spirit/include/classic_switch.hpp>
#include <boost/spirit/include/classic_select.hpp>
#include <boost/spirit/include/classic_closure.hpp>

using namespace BOOST_SPIRIT_CLASSIC_NS;

namespace test_grammars {

///////////////////////////////////////////////////////////////////////////////
//  Test the direct switch_p usage (with default_p)
    struct switch_grammar_direct_default4 
    :   public grammar<switch_grammar_direct_default4>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_direct_default4 const& /*self*/)
            {
                r = switch_p [
                        case_p<'a'>(int_p),
                        case_p<'b'>(ch_p(',')),
                        case_p<'c'>(str_p("bcd")),
                        default_p
                    ];
            }

            rule<ScannerT> r;
            rule<ScannerT> const& start() const { return r; }
        };
    };

    struct switch_grammar_direct_default5 
    :   public grammar<switch_grammar_direct_default5>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_direct_default5 const& /*self*/)
            {
                r = switch_p [
                        case_p<'a'>(int_p),
                        case_p<'b'>(ch_p(',')),
                        default_p,
                        case_p<'c'>(str_p("bcd"))
                    ];
            }

            rule<ScannerT> r;
            rule<ScannerT> const& start() const { return r; }
        };
    };

    struct switch_grammar_direct_default6 
    :   public grammar<switch_grammar_direct_default6>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_direct_default6 const& /*self*/)
            {
                r = switch_p [
                        default_p,
                        case_p<'a'>(int_p),
                        case_p<'b'>(ch_p(',')),
                        case_p<'c'>(str_p("bcd"))
                    ];
            }

            rule<ScannerT> r;
            rule<ScannerT> const& start() const { return r; }
        };
    };

///////////////////////////////////////////////////////////////////////////////
//  Test the switch_p usage given a parser as the switch condition
    struct switch_grammar_parser_default4 
    :   public grammar<switch_grammar_parser_default4>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_parser_default4 const& /*self*/)
            {
                r = switch_p(anychar_p) [
                        case_p<'a'>(int_p),
                        case_p<'b'>(ch_p(',')),
                        case_p<'c'>(str_p("bcd")),
                        default_p
                    ];
            }

            rule<ScannerT> r;
            rule<ScannerT> const& start() const { return r; }
        };
    };

    struct switch_grammar_parser_default5 
    :   public grammar<switch_grammar_parser_default5>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_parser_default5 const& /*self*/)
            {
                r = switch_p(anychar_p) [
                        case_p<'a'>(int_p),
                        case_p<'b'>(ch_p(',')),
                        default_p,
                        case_p<'c'>(str_p("bcd"))
                    ];
            }

            rule<ScannerT> r;
            rule<ScannerT> const& start() const { return r; }
        };
    };

    struct switch_grammar_parser_default6 
    :   public grammar<switch_grammar_parser_default6>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_parser_default6 const& /*self*/)
            {
                r = switch_p(anychar_p) [
                        default_p,
                        case_p<'a'>(int_p),
                        case_p<'b'>(ch_p(',')),
                        case_p<'c'>(str_p("bcd"))
                    ];
            }

            rule<ScannerT> r;
            rule<ScannerT> const& start() const { return r; }
        };
    };

///////////////////////////////////////////////////////////////////////////////
//  Test the switch_p usage given an actor as the switch condition
    struct select_result : public BOOST_SPIRIT_CLASSIC_NS::closure<select_result, int>
    {
        member1 val;
    };

    struct switch_grammar_actor_default4 
    :   public grammar<switch_grammar_actor_default4>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_actor_default4 const& /*self*/)
            {
                using phoenix::arg1;
                r = select_p('a', 'b', 'c', 'd')[r.val = arg1] >>
                    switch_p(r.val) [
                        case_p<0>(int_p),
                        case_p<1>(ch_p(',')),
                        case_p<2>(str_p("bcd")),
                        default_p
                    ];
            }

            rule<ScannerT, select_result::context_t> r;
            rule<ScannerT, select_result::context_t> const& 
            start() const { return r; }
        };
    };

    struct switch_grammar_actor_default5 
    :   public grammar<switch_grammar_actor_default5>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_actor_default5 const& /*self*/)
            {
                using phoenix::arg1;
                r = select_p('a', 'b', 'c', 'd')[r.val = arg1] >>
                    switch_p(r.val) [
                        case_p<0>(int_p),
                        case_p<1>(ch_p(',')),
                        default_p,
                        case_p<2>(str_p("bcd"))
                    ];
            }

            rule<ScannerT, select_result::context_t> r;
            rule<ScannerT, select_result::context_t> const& 
            start() const { return r; }
        };
    };

    struct switch_grammar_actor_default6 
    :   public grammar<switch_grammar_actor_default6>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_actor_default6 const& /*self*/)
            {
                using phoenix::arg1;
                r = select_p('a', 'b', 'c', 'd')[r.val = arg1] >>
                    switch_p(r.val) [
                        default_p,
                        case_p<0>(int_p),
                        case_p<1>(ch_p(',')),
                        case_p<2>(str_p("bcd"))
                    ];
            }

            rule<ScannerT, select_result::context_t> r;
            rule<ScannerT, select_result::context_t> const& 
            start() const { return r; }
        };
    };
    
}   // namespace test_grammars

///////////////////////////////////////////////////////////////////////////////
namespace tests {

    //  Tests for known (to the grammars) sequences
    struct check_grammar_known {
    
        template <typename GrammarT>
        void operator()(GrammarT)
        {
            GrammarT g;
            
            BOOST_TEST(parse("a1", g).full);
            BOOST_TEST(!parse("a,", g).hit);
            BOOST_TEST(!parse("abcd", g).hit);
            
            BOOST_TEST(parse("a 1", g, space_p).full);
            BOOST_TEST(!parse("a ,", g, space_p).hit);
            BOOST_TEST(!parse("a bcd", g, space_p).hit);
            
            BOOST_TEST(!parse("b1", g).hit);
            BOOST_TEST(parse("b,", g).full);
            BOOST_TEST(!parse("bbcd", g).hit);
            
            BOOST_TEST(!parse("b 1", g, space_p).hit);
            BOOST_TEST(parse("b ,", g, space_p).full);
            BOOST_TEST(!parse("b bcd", g, space_p).hit);
            
            BOOST_TEST(!parse("c1", g).hit);
            BOOST_TEST(!parse("c,", g).hit);
            BOOST_TEST(parse("cbcd", g).full);
            
            BOOST_TEST(!parse("c 1", g, space_p).hit);
            BOOST_TEST(!parse("c ,", g, space_p).hit);
            BOOST_TEST(parse("c bcd", g, space_p).full);
        }
    };

    //  Tests for known (to the grammars) sequences
    //  Tests for the default branches (without parsers) of the grammars
    struct check_grammar_default_plain {
    
        template <typename GrammarT>
        void operator()(GrammarT)
        {
            GrammarT g;
            
            BOOST_TEST(parse("d", g).full);
            BOOST_TEST(parse(" d", g, space_p).full); // JDG 10-18-2005 removed trailing ' ' to
                                                  // avoid post skip problems
        }
    };
    
}   // namespace tests

int 
main()
{
    //  Test switch_p parsers containing special (epsilon) default_p case 
    //  branches
    typedef boost::mpl::list<
    // switch_p syntax
        test_grammars::switch_grammar_direct_default4,
        test_grammars::switch_grammar_direct_default5,
        test_grammars::switch_grammar_direct_default6,
        
    // switch_p(parser) syntax
        test_grammars::switch_grammar_parser_default4,
        test_grammars::switch_grammar_parser_default5,
        test_grammars::switch_grammar_parser_default6,
        
    // switch_p(actor) syntax
        test_grammars::switch_grammar_actor_default4,
        test_grammars::switch_grammar_actor_default5,
        test_grammars::switch_grammar_actor_default6
    > default_epsilon_list_t;
    
    boost::mpl::for_each<default_epsilon_list_t>(tests::check_grammar_known());
    boost::mpl::for_each<default_epsilon_list_t>(
        tests::check_grammar_default_plain());

    return boost::report_errors();
}
