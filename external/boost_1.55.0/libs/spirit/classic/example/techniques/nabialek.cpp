/*=============================================================================
    Copyright (c) 2003 Sam Nabialek
    Copyright (c) 2003-2004 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <iostream>

#define BOOST_SPIRIT_DEBUG

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_error_handling.hpp>
#include <boost/spirit/include/classic_iterator.hpp>
#include <boost/spirit/include/classic_symbols.hpp>
#include <boost/spirit/include/classic_utility.hpp>

using namespace BOOST_SPIRIT_CLASSIC_NS;

template <typename Rule>
struct SetRest
{
    SetRest(Rule& the_rule)
    : m_the_rule(the_rule)
    {
    }

    void operator()(Rule* new_rule) const
    {
        m_the_rule = *new_rule;
    }

private:

    Rule& m_the_rule;
};


struct nabialek_trick : public grammar<nabialek_trick>
{
    template <typename ScannerT>
    struct definition
    {
        typedef rule<ScannerT> rule_t;

        rule_t name;
        rule_t line;
        rule_t rest;
        rule_t main;
        rule_t one;
        rule_t two;

        symbols<rule_t*> continuations;

        definition(nabialek_trick const& self)
        {
            name = lexeme_d[repeat_p(1,20)[alnum_p | '_']];

            one = name;
            two = name >> ',' >> name;

            continuations.add
                ("one", &one)
                ("two", &two)
                ;

            line = continuations[SetRest<rule_t>(rest)] >> rest;
            main = *line;

            BOOST_SPIRIT_DEBUG_RULE(name);
            BOOST_SPIRIT_DEBUG_RULE(line);
            BOOST_SPIRIT_DEBUG_RULE(rest);
            BOOST_SPIRIT_DEBUG_RULE(main);
            BOOST_SPIRIT_DEBUG_RULE(one);
            BOOST_SPIRIT_DEBUG_RULE(two);
        }

        rule_t const&
        start() const
        {
            return main;
        }
    };
};

int
main()
{
    nabialek_trick g;
    parse("one only\none again\ntwo first,second", g, space_p);
    return 0;
}
