//
//  Copyright (c) 2005 Joao Abecasis
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_attribute.hpp>
#include <boost/spirit/include/classic_grammar_def.hpp> 

struct my_grammar1
    : BOOST_SPIRIT_CLASSIC_NS::grammar<my_grammar1>
{
    template <typename Scanner>
    struct definition
        : BOOST_SPIRIT_CLASSIC_NS::grammar_def<
            BOOST_SPIRIT_CLASSIC_NS::rule<Scanner>,
            BOOST_SPIRIT_CLASSIC_NS::same
        >
    {
        definition(my_grammar1 const &)
        {
            BOOST_SPIRIT_DEBUG_NODE(start_rule1);
            BOOST_SPIRIT_DEBUG_NODE(start_rule2);

            start_rule1 = BOOST_SPIRIT_CLASSIC_NS::str_p("int");
            start_rule2 = BOOST_SPIRIT_CLASSIC_NS::int_p;

            this->start_parsers(start_rule1, start_rule2);
        }

        BOOST_SPIRIT_CLASSIC_NS::rule<Scanner>
            start_rule1,
            start_rule2;
    };
};

struct my_closure : BOOST_SPIRIT_CLASSIC_NS::closure<my_closure, int>
{
    member1 value;
};

struct my_grammar2
    : BOOST_SPIRIT_CLASSIC_NS::grammar<my_grammar2, my_closure::context_t>
{
    template <typename Scanner>
    struct definition
        : BOOST_SPIRIT_CLASSIC_NS::grammar_def<
            BOOST_SPIRIT_CLASSIC_NS::rule<Scanner>,
            BOOST_SPIRIT_CLASSIC_NS::same
        >
    {
        definition(my_grammar2 const &)
        {
            BOOST_SPIRIT_DEBUG_NODE(start_rule1);
            BOOST_SPIRIT_DEBUG_NODE(start_rule2);

            start_rule1 = BOOST_SPIRIT_CLASSIC_NS::str_p("int");
            start_rule2 = BOOST_SPIRIT_CLASSIC_NS::int_p;

            this->start_parsers(start_rule1, start_rule2);
        }

        BOOST_SPIRIT_CLASSIC_NS::rule<Scanner>
            start_rule1,
            start_rule2;
    };
};

int main()
{
    my_grammar1 g1;
    my_grammar2 g2;

    BOOST_SPIRIT_DEBUG_NODE(g1);
    BOOST_SPIRIT_DEBUG_NODE(g2);

    parse(
        "int 5",
        g1.use_parser<0>() >> g2.use_parser<1>(),
        BOOST_SPIRIT_CLASSIC_NS::space_p
    );
}
