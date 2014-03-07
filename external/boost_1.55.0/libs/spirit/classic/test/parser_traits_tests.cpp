/*=============================================================================
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  Parser traits tests
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/detail/lightweight_test.hpp>
#include <iostream>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>

using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

typedef ref_value_actor<char, assign_action> assign_actor;

//  Test parser_traits templates
void
parser_traits_tests()
{
// is_parser_category_tests
    typedef chlit<char> plain_t;
    typedef optional<chlit<char> > unary_t;
    typedef action<chlit<char>, assign_actor> action_t;
    typedef sequence<chlit<char>, anychar_parser> binary_t;

// is_parser
    BOOST_STATIC_ASSERT(is_parser<plain_t>::value);
    BOOST_STATIC_ASSERT(is_parser<unary_t>::value);
    BOOST_STATIC_ASSERT(is_parser<action_t>::value);
    BOOST_STATIC_ASSERT(is_parser<binary_t>::value);

// is_action_parser
    BOOST_STATIC_ASSERT(!is_action_parser<plain_t>::value);
    BOOST_STATIC_ASSERT(!is_action_parser<unary_t>::value);
    BOOST_STATIC_ASSERT(is_action_parser<action_t>::value);
    BOOST_STATIC_ASSERT(!is_action_parser<binary_t>::value);

// is_unary_composite
    BOOST_STATIC_ASSERT(!is_unary_composite<plain_t>::value);
    BOOST_STATIC_ASSERT(is_unary_composite<unary_t>::value);
    BOOST_STATIC_ASSERT(is_unary_composite<action_t>::value);   // action_t _is_ an unary_t!
    BOOST_STATIC_ASSERT(!is_unary_composite<binary_t>::value);

// is_binary_composite
    BOOST_STATIC_ASSERT(!is_binary_composite<plain_t>::value);
    BOOST_STATIC_ASSERT(!is_binary_composite<unary_t>::value);
    BOOST_STATIC_ASSERT(!is_binary_composite<action_t>::value);
    BOOST_STATIC_ASSERT(is_binary_composite<binary_t>::value);

// is_composite_parser
    BOOST_STATIC_ASSERT(!is_composite_parser<plain_t>::value);
    BOOST_STATIC_ASSERT(is_composite_parser<unary_t>::value);
    BOOST_STATIC_ASSERT(is_composite_parser<action_t>::value);   // action_t _is_ an unary_t!
    BOOST_STATIC_ASSERT(is_composite_parser<binary_t>::value);

// is_composite_type_tests
    typedef alternative<chlit<char>, anychar_parser> alternative_t;
    typedef sequence<chlit<char>, anychar_parser> sequence_t;
    typedef sequential_or<chlit<char>, anychar_parser> sequential_or_t;
    typedef intersection<chlit<char>, anychar_parser> intersection_t;
    typedef difference<chlit<char>, anychar_parser> difference_t;
    typedef exclusive_or<chlit<char>, anychar_parser> exclusive_or_t;

    typedef optional<chlit<char> > optional_t;
    typedef kleene_star<chlit<char> > kleene_star_t;
    typedef positive<chlit<char> > positive_t;

// is_parser
    BOOST_STATIC_ASSERT(is_parser<alternative_t>::value);
    BOOST_STATIC_ASSERT(is_parser<sequence_t>::value);
    BOOST_STATIC_ASSERT(is_parser<sequential_or_t>::value);
    BOOST_STATIC_ASSERT(is_parser<intersection_t>::value);
    BOOST_STATIC_ASSERT(is_parser<difference_t>::value);
    BOOST_STATIC_ASSERT(is_parser<exclusive_or_t>::value);
    BOOST_STATIC_ASSERT(is_parser<optional_t>::value);
    BOOST_STATIC_ASSERT(is_parser<positive_t>::value);

// is_alternative
    BOOST_STATIC_ASSERT(!is_alternative<plain_t>::value);
    BOOST_STATIC_ASSERT(!is_alternative<action_t>::value);

    BOOST_STATIC_ASSERT(is_alternative<alternative_t>::value);
    BOOST_STATIC_ASSERT(!is_alternative<sequence_t>::value);
    BOOST_STATIC_ASSERT(!is_alternative<sequential_or_t>::value);
    BOOST_STATIC_ASSERT(!is_alternative<intersection_t>::value);
    BOOST_STATIC_ASSERT(!is_alternative<difference_t>::value);
    BOOST_STATIC_ASSERT(!is_alternative<exclusive_or_t>::value);

    BOOST_STATIC_ASSERT(!is_alternative<optional_t>::value);
    BOOST_STATIC_ASSERT(!is_alternative<kleene_star_t>::value);
    BOOST_STATIC_ASSERT(!is_alternative<positive_t>::value);

// is_sequence
    BOOST_STATIC_ASSERT(!is_sequence<plain_t>::value);
    BOOST_STATIC_ASSERT(!is_sequence<action_t>::value);

    BOOST_STATIC_ASSERT(!is_sequence<alternative_t>::value);
    BOOST_STATIC_ASSERT(is_sequence<sequence_t>::value);
    BOOST_STATIC_ASSERT(!is_sequence<sequential_or_t>::value);
    BOOST_STATIC_ASSERT(!is_sequence<intersection_t>::value);
    BOOST_STATIC_ASSERT(!is_sequence<difference_t>::value);
    BOOST_STATIC_ASSERT(!is_sequence<exclusive_or_t>::value);

    BOOST_STATIC_ASSERT(!is_sequence<optional_t>::value);
    BOOST_STATIC_ASSERT(!is_sequence<kleene_star_t>::value);
    BOOST_STATIC_ASSERT(!is_sequence<positive_t>::value);

// is_sequential_or
    BOOST_STATIC_ASSERT(!is_sequential_or<plain_t>::value);
    BOOST_STATIC_ASSERT(!is_sequential_or<action_t>::value);

    BOOST_STATIC_ASSERT(!is_sequential_or<alternative_t>::value);
    BOOST_STATIC_ASSERT(!is_sequential_or<sequence_t>::value);
    BOOST_STATIC_ASSERT(is_sequential_or<sequential_or_t>::value);
    BOOST_STATIC_ASSERT(!is_sequential_or<intersection_t>::value);
    BOOST_STATIC_ASSERT(!is_sequential_or<difference_t>::value);
    BOOST_STATIC_ASSERT(!is_sequential_or<exclusive_or_t>::value);

    BOOST_STATIC_ASSERT(!is_sequential_or<optional_t>::value);
    BOOST_STATIC_ASSERT(!is_sequential_or<kleene_star_t>::value);
    BOOST_STATIC_ASSERT(!is_sequential_or<positive_t>::value);

// is_intersection
    BOOST_STATIC_ASSERT(!is_intersection<plain_t>::value);
    BOOST_STATIC_ASSERT(!is_intersection<action_t>::value);

    BOOST_STATIC_ASSERT(!is_intersection<alternative_t>::value);
    BOOST_STATIC_ASSERT(!is_intersection<sequence_t>::value);
    BOOST_STATIC_ASSERT(!is_intersection<sequential_or_t>::value);
    BOOST_STATIC_ASSERT(is_intersection<intersection_t>::value);
    BOOST_STATIC_ASSERT(!is_intersection<difference_t>::value);
    BOOST_STATIC_ASSERT(!is_intersection<exclusive_or_t>::value);

    BOOST_STATIC_ASSERT(!is_intersection<optional_t>::value);
    BOOST_STATIC_ASSERT(!is_intersection<kleene_star_t>::value);
    BOOST_STATIC_ASSERT(!is_intersection<positive_t>::value);

// is_difference
    BOOST_STATIC_ASSERT(!is_difference<plain_t>::value);
    BOOST_STATIC_ASSERT(!is_difference<action_t>::value);

    BOOST_STATIC_ASSERT(!is_difference<alternative_t>::value);
    BOOST_STATIC_ASSERT(!is_difference<sequence_t>::value);
    BOOST_STATIC_ASSERT(!is_difference<sequential_or_t>::value);
    BOOST_STATIC_ASSERT(!is_difference<intersection_t>::value);
    BOOST_STATIC_ASSERT(is_difference<difference_t>::value);
    BOOST_STATIC_ASSERT(!is_difference<exclusive_or_t>::value);

    BOOST_STATIC_ASSERT(!is_difference<optional_t>::value);
    BOOST_STATIC_ASSERT(!is_difference<kleene_star_t>::value);
    BOOST_STATIC_ASSERT(!is_difference<positive_t>::value);

// is_exclusive_or
    BOOST_STATIC_ASSERT(!is_exclusive_or<plain_t>::value);
    BOOST_STATIC_ASSERT(!is_exclusive_or<action_t>::value);

    BOOST_STATIC_ASSERT(!is_exclusive_or<alternative_t>::value);
    BOOST_STATIC_ASSERT(!is_exclusive_or<sequence_t>::value);
    BOOST_STATIC_ASSERT(!is_exclusive_or<sequential_or_t>::value);
    BOOST_STATIC_ASSERT(!is_exclusive_or<intersection_t>::value);
    BOOST_STATIC_ASSERT(!is_exclusive_or<difference_t>::value);
    BOOST_STATIC_ASSERT(is_exclusive_or<exclusive_or_t>::value);

    BOOST_STATIC_ASSERT(!is_exclusive_or<optional_t>::value);
    BOOST_STATIC_ASSERT(!is_exclusive_or<kleene_star_t>::value);
    BOOST_STATIC_ASSERT(!is_exclusive_or<positive_t>::value);

// is_optional
    BOOST_STATIC_ASSERT(!is_optional<plain_t>::value);
    BOOST_STATIC_ASSERT(!is_optional<action_t>::value);

    BOOST_STATIC_ASSERT(!is_optional<alternative_t>::value);
    BOOST_STATIC_ASSERT(!is_optional<sequence_t>::value);
    BOOST_STATIC_ASSERT(!is_optional<sequential_or_t>::value);
    BOOST_STATIC_ASSERT(!is_optional<intersection_t>::value);
    BOOST_STATIC_ASSERT(!is_optional<difference_t>::value);
    BOOST_STATIC_ASSERT(!is_optional<exclusive_or_t>::value);

    BOOST_STATIC_ASSERT(is_optional<optional_t>::value);
    BOOST_STATIC_ASSERT(!is_optional<kleene_star_t>::value);
    BOOST_STATIC_ASSERT(!is_optional<positive_t>::value);

// is_kleene_star
    BOOST_STATIC_ASSERT(!is_kleene_star<plain_t>::value);
    BOOST_STATIC_ASSERT(!is_kleene_star<action_t>::value);

    BOOST_STATIC_ASSERT(!is_kleene_star<alternative_t>::value);
    BOOST_STATIC_ASSERT(!is_kleene_star<sequence_t>::value);
    BOOST_STATIC_ASSERT(!is_kleene_star<sequential_or_t>::value);
    BOOST_STATIC_ASSERT(!is_kleene_star<intersection_t>::value);
    BOOST_STATIC_ASSERT(!is_kleene_star<difference_t>::value);
    BOOST_STATIC_ASSERT(!is_kleene_star<exclusive_or_t>::value);

    BOOST_STATIC_ASSERT(!is_kleene_star<optional_t>::value);
    BOOST_STATIC_ASSERT(is_kleene_star<kleene_star_t>::value);
    BOOST_STATIC_ASSERT(!is_kleene_star<positive_t>::value);

// is_positive
    BOOST_STATIC_ASSERT(!is_positive<plain_t>::value);
    BOOST_STATIC_ASSERT(!is_positive<action_t>::value);

    BOOST_STATIC_ASSERT(!is_positive<alternative_t>::value);
    BOOST_STATIC_ASSERT(!is_positive<sequence_t>::value);
    BOOST_STATIC_ASSERT(!is_positive<sequential_or_t>::value);
    BOOST_STATIC_ASSERT(!is_positive<intersection_t>::value);
    BOOST_STATIC_ASSERT(!is_positive<difference_t>::value);
    BOOST_STATIC_ASSERT(!is_positive<exclusive_or_t>::value);

    BOOST_STATIC_ASSERT(!is_positive<optional_t>::value);
    BOOST_STATIC_ASSERT(!is_positive<kleene_star_t>::value);
    BOOST_STATIC_ASSERT(is_positive<positive_t>::value);
}

///////////////////////////////////////////////////////////////////////////////
//  Test parser extraction templates
void
parser_extraction_tests()
{
    typedef chlit<char> plain_t;
    typedef optional<chlit<char> > unary_t;
    typedef action<chlit<char>, assign_actor> action_t;
    typedef sequence<chlit<char>, anychar_parser> binary_t;

// parser type extraction templates
    BOOST_STATIC_ASSERT((
        ::boost::is_same<
                chlit<char>, unary_subject<unary_t>::type
            >::value));
    BOOST_STATIC_ASSERT((
        ::boost::is_same<
                chlit<char>, action_subject<action_t>::type
            >::value));
    BOOST_STATIC_ASSERT((
        ::boost::is_same<
                assign_actor, semantic_action<action_t>::type
            >::value));
    BOOST_STATIC_ASSERT((
        ::boost::is_same<
                chlit<char>, binary_left_subject<binary_t>::type
            >::value));
    BOOST_STATIC_ASSERT((
        ::boost::is_same<
                anychar_parser, binary_right_subject<binary_t>::type
            >::value));

// parser object extraction functions
    BOOST_TEST(1 == parse("aaaa", get_unary_subject(*ch_p('a'))).length);

char c = 'b';

    BOOST_TEST(1 == parse("aaaa", get_action_subject(ch_p('a')[assign(c)])).length);
    BOOST_TEST(c == 'b');

    BOOST_TEST(1 == parse("aaaa",
        ch_p('a')[ get_semantic_action(ch_p('b')[assign(c)]) ]).length);
    BOOST_TEST(c == 'a');

    BOOST_TEST(1 == parse("abab",
        get_binary_left_subject(ch_p('a') >> ch_p('b'))).length);
    BOOST_TEST(1 == parse("baba",
        get_binary_right_subject(ch_p('a') >> ch_p('b'))).length);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Main
//
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    parser_traits_tests();
    parser_extraction_tests();

    return boost::report_errors();
}

