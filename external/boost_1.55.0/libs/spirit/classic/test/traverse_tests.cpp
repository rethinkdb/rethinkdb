/*=============================================================================
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  Traversal tests
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/detail/lightweight_test.hpp>
#include <iostream>
#include <string>
#include <vector>

#include <boost/config.hpp>
#include <boost/static_assert.hpp>

#ifdef BOOST_NO_STRINGSTREAM
#include <strstream>
#define OSSTREAM std::ostrstream
std::string GETSTRING(std::ostrstream& ss)
{
    ss << ends;
    std::string rval = ss.str();
    ss.freeze(false);
    return rval;
}
#else
#include <sstream>
#define GETSTRING(ss) ss.str()
#define OSSTREAM std::ostringstream
#endif

#ifndef BOOST_SPIRIT_DEBUG
#define BOOST_SPIRIT_DEBUG    // needed for parser_name functions
#endif

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
#include <boost/spirit/include/classic_meta.hpp>

using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

typedef ref_value_actor<char, assign_action> assign_actor;

///////////////////////////////////////////////////////////////////////////////
//
//  Test identity transformation
//
///////////////////////////////////////////////////////////////////////////////
void
traverse_identity_tests()
{
    //  test type equality
    typedef sequence<chlit<char>, chlit<char> > test_sequence1_t;
    BOOST_STATIC_ASSERT((
        ::boost::is_same<
            test_sequence1_t,
            post_order::result<identity_transform, test_sequence1_t>::type
        >::value
    ));

    //  test (rough) runtime equality
    BOOST_TEST(
        parse(
            "ab",
            post_order::traverse(identity_transform(), ch_p('a') >> 'b')
        ).full
    );
    BOOST_TEST(
        !parse(
            "ba",
            post_order::traverse(identity_transform(), ch_p('a') >> 'b')
        ).hit
    );

    ///////////////////////////////////////////////////////////////////////////
    BOOST_TEST(
        !parse(
            "cba",
            post_order::traverse(
                identity_transform(),
                ch_p('a') >> 'b' >> 'c'
            )
        ).hit
    );

///////////////////////////////////////////////////////////////////////////////
//  Test more complex sequences
char c;

///////////////////////////////////////////////////////////////////////////////
//  test: ((a >> b) >> c) >> d
    typedef
        sequence<
            sequence<
                sequence<
                    kleene_star<chlit<> >,
                    action<chlit<>, assign_actor>
                >,
                chlit<>
            >,
            optional<chlit<> >
        > test_sequence2_t;

    BOOST_STATIC_ASSERT((
        ::boost::is_same<
            test_sequence2_t,
            post_order::result<identity_transform, test_sequence2_t>::type
        >::value
    ));

    c = 0;
    BOOST_TEST(
        parse(
            "aabcd",
            post_order::traverse(
                identity_transform(),
                ((*ch_p('a') >> ch_p('b')[assign_a(c)]) >> 'c') >> !ch_p('d')
            )
        ).full
    );
    BOOST_TEST(c == 'b');

///////////////////////////////////////////////////////////////////////////////
//  test: (a >> (b >> c)) >> d
    typedef
        sequence<
            sequence<
                kleene_star<chlit<> >,
                sequence<
                    action<chlit<>, assign_actor>,
                    chlit<>
                >
            >,
            optional<chlit<char> >
        > test_sequence3_t;

    BOOST_STATIC_ASSERT((
        ::boost::is_same<
            test_sequence3_t,
            post_order::result<identity_transform, test_sequence3_t>::type
        >::value
    ));

    c = 0;
    BOOST_TEST(
        parse(
            "aabcd",
            post_order::traverse(
                identity_transform(),
                (*ch_p('a') >> (ch_p('b')[assign_a(c)] >> 'c')) >> !ch_p('d')
            )
        ).full
    );
    BOOST_TEST(c == 'b');

///////////////////////////////////////////////////////////////////////////////
//  test: a >> (b >> (c >> d))
    typedef
        sequence<
            kleene_star<chlit<> >,
            sequence<
                action<chlit<>, assign_actor>,
                sequence<
                    chlit<>,
                    optional<chlit<> >
                >
            >
        > test_sequence4_t;

    BOOST_STATIC_ASSERT((
        ::boost::is_same<
            test_sequence4_t,
            post_order::result<identity_transform, test_sequence4_t>::type
        >::value
    ));

    c = 0;
    BOOST_TEST(
        parse(
            "aabcd",
            post_order::traverse(
                identity_transform(),
                *ch_p('a') >> (ch_p('b')[assign_a(c)] >> ('c' >> !ch_p('d')))
            )
        ).full
    );
    BOOST_TEST(c == 'b');

///////////////////////////////////////////////////////////////////////////////
//  test: a >> ((b >> c) >> d)
    typedef
        sequence<
            kleene_star<chlit<> >,
            sequence<
                sequence<
                    action<chlit<>, assign_actor>,
                    chlit<>
                >,
                optional<chlit<> >
            >
        > test_sequence5_t;

    BOOST_STATIC_ASSERT((
        ::boost::is_same<
            test_sequence5_t,
            post_order::result<identity_transform, test_sequence5_t>::type
        >::value
    ));

    c = 0;
    BOOST_TEST(
        parse(
            "aabcd",
            post_order::traverse(
                identity_transform(),
                *ch_p('a') >> ((ch_p('b')[assign_a(c)] >> 'c') >> !ch_p('d'))
            )
        ).full
    );
    BOOST_TEST(c == 'b');

///////////////////////////////////////////////////////////////////////////////
//  test: (a >> b) >> (c >> d)
    typedef
        sequence<
            sequence<
                kleene_star<chlit<> >,
                action<chlit<>, assign_actor>
            >,
            sequence<
                chlit<>,
                optional<chlit<> >
            >
        > test_sequence6_t;

    BOOST_STATIC_ASSERT((
        ::boost::is_same<
            test_sequence6_t,
            post_order::result<identity_transform, test_sequence6_t>::type
        >::value
    ));

    c = 0;
    BOOST_TEST(
        parse(
            "aabcd",
            post_order::traverse(
                identity_transform(),
                (*ch_p('a') >> ch_p('b')[assign_a(c)]) >> ('c' >> !ch_p('d'))
            )
        ).full
    );
    BOOST_TEST(c == 'b');
}

///////////////////////////////////////////////////////////////////////////////
//
//  The following is a tracing identity_transform traverse metafunction
//
///////////////////////////////////////////////////////////////////////////////

class trace_identity_transform
:   public transform_policies<trace_identity_transform> {

public:
    typedef trace_identity_transform self_t;
    typedef transform_policies<trace_identity_transform> base_t;

    template <typename ParserT, typename EnvT>
    typename parser_traversal_plain_result<self_t, ParserT, EnvT>::type
    generate_plain(ParserT const &parser_, EnvT const &env) const
    {
        OSSTREAM strout;
        strout
            << EnvT::node
                << ": plain ("
                << EnvT::level << ", "
                << EnvT::index
                << "): "
            << parser_name(parser_);
        traces.push_back(GETSTRING(strout));

        return this->base_t::generate_plain(parser_, env);
    }

    template <typename UnaryT, typename SubjectT, typename EnvT>
    typename parser_traversal_unary_result<self_t, UnaryT, SubjectT, EnvT>::type
    generate_unary(UnaryT const &unary_, SubjectT const &subject_,
        EnvT const &env) const
    {
        OSSTREAM strout;
        strout
            << EnvT::node << ": unary ("
                << EnvT::level
                << "): "
            << parser_name(unary_);
        traces.push_back(GETSTRING(strout));

        return this->base_t::generate_unary(unary_, subject_, env);
    }

    template <typename ActionT, typename SubjectT, typename EnvT>
    typename parser_traversal_action_result<self_t, ActionT, SubjectT, EnvT>::type
    generate_action(ActionT const &action_, SubjectT const &subject_,
        EnvT const &env) const
    {
        OSSTREAM strout;
        strout
            << EnvT::node << ": action("
                << EnvT::level
                << "): "
            << parser_name(action_);
        traces.push_back(GETSTRING(strout));

        return this->base_t::generate_action(action_, subject_, env);
    }

    template <typename BinaryT, typename LeftT, typename RightT, typename EnvT>
    typename parser_traversal_binary_result<self_t, BinaryT, LeftT, RightT, EnvT>::type
    generate_binary(BinaryT const &binary_, LeftT const& left_,
        RightT const& right_, EnvT const &env) const
    {
        OSSTREAM strout;
        strout
            << EnvT::node << ": binary("
                << EnvT::level
                << "): "
            << parser_name(binary_);
        traces.push_back(GETSTRING(strout));

        return this->base_t::generate_binary(binary_, left_, right_, env);
    }

    std::vector<string> const &get_output() const { return traces; }

private:
    mutable std::vector<std::string> traces;
};

template <typename ParserT>
void
post_order_trace_test(ParserT const &parser_, char const *first[], size_t cnt)
{
// traverse
trace_identity_transform trace_vector;

    post_order::traverse(trace_vector, parser_);

// The following two re-find loops ensure, that both string arrays contain the
// same entries, only their order may differ. The differences in the trace
// string order is based on the different parameter evaluation order as it is
// implemented by different compilers.

// re-find all trace strings in the array of expected strings
std::vector<std::string>::const_iterator it = trace_vector.get_output().begin();
std::vector<std::string>::const_iterator end = trace_vector.get_output().end();

    BOOST_TEST(cnt == trace_vector.get_output().size());
    for (/**/;  it != end; ++it)
    {
        if (std::find(first, first + cnt, *it) == first + cnt)
            cerr << "node in question: " << *it << endl;

        BOOST_TEST(std::find(first, first + cnt, *it) != first + cnt);
    }

// re-find all expected strings in the vector of trace strings
std::vector<std::string>::const_iterator begin = trace_vector.get_output().begin();
char const *expected = first[0];

    for (size_t i = 0; i < cnt; expected = first[++i])
    {
        if (std::find(begin, end, std::string(expected)) == end)
            cerr << "node in question: " << expected << endl;

        BOOST_TEST(std::find(begin, end, std::string(expected)) != end);
    }
}

#if defined(_countof)
#undef  _countof
#endif
#define _countof(x) (sizeof(x)/sizeof(x[0]))

void
traverse_trace_tests()
{
const char *test_result1[] = {
        "0: plain (1, 0): chlit('a')",
        "1: plain (1, 1): chlit('b')",
        "2: binary(0): sequence[chlit('a'), chlit('b')]",
    };

    post_order_trace_test(
        ch_p('a') >> 'b',
        test_result1, _countof(test_result1)
    );

char c = 0;

// test: ((a >> b) >> c) >> d
const char *test_result2[] = {
        "0: plain (4, 0): chlit('a')",
        "1: unary (3): kleene_star[chlit('a')]",
        "2: plain (4, 1): chlit('b')",
        "3: action(3): action[chlit('b')]",
        "4: binary(2): sequence[kleene_star[chlit('a')], action[chlit('b')]]",
        "5: plain (2, 2): chlit('c')",
        "6: binary(1): sequence[sequence[kleene_star[chlit('a')], action[chlit('b')]], chlit('c')]",
        "7: plain (2, 3): chlit('d')",
        "8: unary (1): optional[chlit('d')]",
        "9: binary(0): sequence[sequence[sequence[kleene_star[chlit('a')], action[chlit('b')]], chlit('c')], optional[chlit('d')]]",
    };

    post_order_trace_test(
        ((*ch_p('a') >> ch_p('b')[assign_a(c)]) >> 'c') >> !ch_p('d'),
        test_result2, _countof(test_result2)
    );

// test: (a >> (b >> c)) >> d
const char *test_result3[] = {
        "0: plain (3, 0): chlit('a')",
        "1: unary (2): kleene_star[chlit('a')]",
        "2: plain (4, 1): chlit('b')",
        "3: action(3): action[chlit('b')]",
        "4: plain (3, 2): chlit('c')",
        "5: binary(2): sequence[action[chlit('b')], chlit('c')]",
        "6: binary(1): sequence[kleene_star[chlit('a')], sequence[action[chlit('b')], chlit('c')]]",
        "7: plain (2, 3): chlit('d')",
        "8: unary (1): optional[chlit('d')]",
        "9: binary(0): sequence[sequence[kleene_star[chlit('a')], sequence[action[chlit('b')], chlit('c')]], optional[chlit('d')]]",
    };

    post_order_trace_test(
        (*ch_p('a') >> (ch_p('b')[assign_a(c)] >> 'c')) >> !ch_p('d'),
        test_result3, _countof(test_result3)
    );

// test: a >> (b >> (c >> d))
const char *test_result4[] = {
        "0: plain (2, 0): chlit('a')",
        "1: unary (1): kleene_star[chlit('a')]",
        "2: plain (3, 1): chlit('b')",
        "3: action(2): action[chlit('b')]",
        "4: plain (3, 2): chlit('c')",
        "5: plain (4, 3): chlit('d')",
        "6: unary (3): optional[chlit('d')]",
        "7: binary(2): sequence[chlit('c'), optional[chlit('d')]]",
        "8: binary(1): sequence[action[chlit('b')], sequence[chlit('c'), optional[chlit('d')]]]",
        "9: binary(0): sequence[kleene_star[chlit('a')], sequence[action[chlit('b')], sequence[chlit('c'), optional[chlit('d')]]]]",
    };

    post_order_trace_test(
        *ch_p('a') >> (ch_p('b')[assign_a(c)] >> ('c' >> !ch_p('d'))),
        test_result4, _countof(test_result4)
    );

// test: a >> ((b >> c) >> d)
const char *test_result5[] = {
        "0: plain (2, 0): chlit('a')",
        "1: unary (1): kleene_star[chlit('a')]",
        "2: plain (4, 1): chlit('b')",
        "3: action(3): action[chlit('b')]",
        "4: plain (3, 2): chlit('c')",
        "5: binary(2): sequence[action[chlit('b')], chlit('c')]",
        "6: plain (3, 3): chlit('d')",
        "7: unary (2): optional[chlit('d')]",
        "8: binary(1): sequence[sequence[action[chlit('b')], chlit('c')], optional[chlit('d')]]",
        "9: binary(0): sequence[kleene_star[chlit('a')], sequence[sequence[action[chlit('b')], chlit('c')], optional[chlit('d')]]]",
    };

    post_order_trace_test(
        *ch_p('a') >> ((ch_p('b')[assign_a(c)] >> 'c') >> !ch_p('d')),
        test_result5, _countof(test_result5)
    );

// test: (a >> b) >> (c >> d)
const char *test_result6[] = {
        "0: plain (3, 0): chlit('a')",
        "1: unary (2): kleene_star[chlit('a')]",
        "2: plain (3, 1): chlit('b')",
        "3: action(2): action[chlit('b')]",
        "4: binary(1): sequence[kleene_star[chlit('a')], action[chlit('b')]]",
        "5: plain (2, 2): chlit('c')",
        "6: plain (3, 3): chlit('d')",
        "7: unary (2): optional[chlit('d')]",
        "8: binary(1): sequence[chlit('c'), optional[chlit('d')]]",
        "9: binary(0): sequence[sequence[kleene_star[chlit('a')], action[chlit('b')]], sequence[chlit('c'), optional[chlit('d')]]]",
    };

    post_order_trace_test(
        (*ch_p('a') >> ch_p('b')[assign_a(c)]) >> ('c' >> !ch_p('d')),
        test_result6, _countof(test_result6)
    );
}

///////////////////////////////////////////////////////////////////////////////
//
//  Main
//
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    traverse_identity_tests();
    traverse_trace_tests();

    return boost::report_errors();
}

