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
//  Test the direct switch_p usage
    struct switch_grammar_direct_single 
    :   public grammar<switch_grammar_direct_single>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_direct_single const& /*self*/)
            {
                r = switch_p [
                        case_p<'a'>(int_p)
                    ];
            }

            rule<ScannerT> r;
            rule<ScannerT> const& start() const { return r; }
        };
    };

    struct switch_grammar_direct_default_single1 
    :   public grammar<switch_grammar_direct_default_single1>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_direct_default_single1 const& /*self*/)
            {
                r = switch_p [
                        default_p(str_p("default"))
                    ];
            }

            rule<ScannerT> r;
            rule<ScannerT> const& start() const { return r; }
        };
    };

    struct switch_grammar_direct_default_single2 
    :   public grammar<switch_grammar_direct_default_single2>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_direct_default_single2 const& /*self*/)
            {
                r = switch_p [
                        default_p
                    ];
            }

            rule<ScannerT> r;
            rule<ScannerT> const& start() const { return r; }
        };
    };

///////////////////////////////////////////////////////////////////////////////
//  Test the switch_p usage given a parser as the switch condition
    struct switch_grammar_parser_single 
    :   public grammar<switch_grammar_parser_single>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_parser_single const& /*self*/)
            {
                r = switch_p(anychar_p) [
                        case_p<'a'>(int_p)
                    ];
            }

            rule<ScannerT> r;
            rule<ScannerT> const& start() const { return r; }
        };
    };

    struct switch_grammar_parser_default_single1 
    :   public grammar<switch_grammar_parser_default_single1>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_parser_default_single1 const& /*self*/)
            {
                r = switch_p(anychar_p) [
                        default_p(str_p("default"))
                    ];
            }

            rule<ScannerT> r;
            rule<ScannerT> const& start() const { return r; }
        };
    };

    struct switch_grammar_parser_default_single2 
    :   public grammar<switch_grammar_parser_default_single2>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_parser_default_single2 const& /*self*/)
            {
                r = switch_p(anychar_p) [
                        default_p
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

    struct switch_grammar_actor_single 
    :   public grammar<switch_grammar_actor_single>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_actor_single const& /*self*/)
            {
                using phoenix::arg1;
                r = select_p('a')[r.val = arg1] >>
                    switch_p(r.val) [
                        case_p<0>(int_p)
                    ];
            }

            rule<ScannerT, select_result::context_t> r;
            rule<ScannerT, select_result::context_t> const& 
            start() const { return r; }
        };
    };

    struct switch_grammar_actor_default_single1 
    :   public grammar<switch_grammar_actor_default_single1>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_actor_default_single1 const& /*self*/)
            {
                using phoenix::arg1;
                r = select_p('d')[r.val = arg1] >>
                    switch_p(r.val) [
                        default_p(str_p("default"))
                    ];
            }

            rule<ScannerT, select_result::context_t> r;
            rule<ScannerT, select_result::context_t> const& 
            start() const { return r; }
        };
    };

    struct switch_grammar_actor_default_single2 
    :   public grammar<switch_grammar_actor_default_single2>
    {
        template <typename ScannerT>
        struct definition 
        {
            definition(switch_grammar_actor_default_single2 const& /*self*/)
            {
                using phoenix::arg1;
                r = select_p('d')[r.val = arg1] >>
                    switch_p(r.val) [
                        default_p
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
    struct check_grammar_unknown {
    
        template <typename GrammarT>
        void operator()(GrammarT)
        {
            GrammarT g;
            
            BOOST_TEST(!parse("a1", g).hit);
            BOOST_TEST(!parse("a,", g).hit);
            BOOST_TEST(!parse("abcd", g).hit);
            
            BOOST_TEST(!parse("a 1", g, space_p).hit);
            BOOST_TEST(!parse("a ,", g, space_p).hit);
            BOOST_TEST(!parse("a bcd", g, space_p).hit);
            
            BOOST_TEST(!parse("b1", g).hit);
            BOOST_TEST(!parse("b,", g).hit);
            BOOST_TEST(!parse("bbcd", g).hit);
            
            BOOST_TEST(!parse("b 1", g, space_p).hit);
            BOOST_TEST(!parse("b ,", g, space_p).hit);
            BOOST_TEST(!parse("b bcd", g, space_p).hit);
            
            BOOST_TEST(!parse("c1", g).hit);
            BOOST_TEST(!parse("c,", g).hit);
            BOOST_TEST(!parse("cbcd", g).hit);
            
            BOOST_TEST(!parse("c 1", g, space_p).hit);
            BOOST_TEST(!parse("c ,", g, space_p).hit);
            BOOST_TEST(!parse("c bcd", g, space_p).hit);
        }
    };

    //  Tests for the default branches (with parsers) of the grammars
    struct check_grammar_default {
    
        template <typename GrammarT>
        void operator()(GrammarT)
        {
            GrammarT g;
            
            BOOST_TEST(parse("ddefault", g).full);
            BOOST_TEST(parse("d default", g, space_p).full);
        }
    };
    
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
    
    //  Tests grammars with a single case_p branch
    struct check_grammar_single {
    
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
            BOOST_TEST(!parse("b,", g).hit);
            BOOST_TEST(!parse("bbcd", g).hit);
            
            BOOST_TEST(!parse("b 1", g, space_p).hit);
            BOOST_TEST(!parse("b ,", g, space_p).hit);
            BOOST_TEST(!parse("b bcd", g, space_p).hit);
            
            BOOST_TEST(!parse("c1", g).hit);
            BOOST_TEST(!parse("c,", g).hit);
            BOOST_TEST(!parse("cbcd", g).hit);
            
            BOOST_TEST(!parse("c 1", g, space_p).hit);
            BOOST_TEST(!parse("c ,", g, space_p).hit);
            BOOST_TEST(!parse("c bcd", g, space_p).hit);
        }
    };
    
}   // namespace tests

int 
main()
{
    // Test switch_p with a single case_p branch
    typedef boost::mpl::list<
        test_grammars::switch_grammar_direct_single,
        test_grammars::switch_grammar_parser_single,
        test_grammars::switch_grammar_actor_single
    > single_list_t;
    
    boost::mpl::for_each<single_list_t>(tests::check_grammar_single());
    
    typedef boost::mpl::list<
        test_grammars::switch_grammar_direct_default_single1,
        test_grammars::switch_grammar_parser_default_single1,
        test_grammars::switch_grammar_actor_default_single1
    > default_single_t;
    
    boost::mpl::for_each<default_single_t>(tests::check_grammar_default());
    boost::mpl::for_each<default_single_t>(tests::check_grammar_unknown());
    
    typedef boost::mpl::list<
        test_grammars::switch_grammar_direct_default_single2,
        test_grammars::switch_grammar_parser_default_single2,
        test_grammars::switch_grammar_actor_default_single2
    > default_plain_single_t;
    
    boost::mpl::for_each<default_plain_single_t>(
        tests::check_grammar_default_plain());
    
    return boost::report_errors();
}
