/*=============================================================================
    Copyright (c) 2005 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <iostream>

#define BOOST_SPIRIT_DEBUG
#include <boost/spirit/include/classic_core.hpp>
#include <boost/assert.hpp>

using namespace BOOST_SPIRIT_CLASSIC_NS;

struct non_greedy_kleene : public grammar<non_greedy_kleene>
{
    template <typename ScannerT>
    struct definition
    {
        typedef rule<ScannerT> rule_t;
        rule_t r;

        definition(non_greedy_kleene const& self)
        {
            r = (alnum_p >> r) | digit_p;
            BOOST_SPIRIT_DEBUG_RULE(r);
        }

        rule_t const&
        start() const
        {
            return r;
        }
    };
};

struct non_greedy_plus : public grammar<non_greedy_plus>
{
    template <typename ScannerT>
    struct definition
    {
        typedef rule<ScannerT> rule_t;
        rule_t r;

        definition(non_greedy_plus const& self)
        {
            r = alnum_p >> (r | digit_p);
            BOOST_SPIRIT_DEBUG_RULE(r);
        }

        rule_t const&
        start() const
        {
            return r;
        }
    };
};
int
main()
{
    bool success;
    {
        non_greedy_kleene k;
        success = parse("3", k).full;
        BOOST_ASSERT(success);
        success = parse("abcdef3", k).full;
        BOOST_ASSERT(success);
        success = parse("abc2def3", k).full;
        BOOST_ASSERT(success);
        success = parse("abc", k).full;
        BOOST_ASSERT(!success);
    }
    
    {
        non_greedy_plus p;
        success = parse("3", p).full;
        BOOST_ASSERT(!success);
        success = parse("abcdef3", p).full;
        BOOST_ASSERT(success);
        success = parse("abc2def3", p).full;
        BOOST_ASSERT(success);
        success = parse("abc", p).full;
        BOOST_ASSERT(!success);
    }

    std::cout << "SUCCESS!!!\n";
    return 0;
}
