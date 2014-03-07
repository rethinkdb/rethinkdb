/*=============================================================================
    Copyright (c) 2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_for.hpp>

extern bool fun();

struct ftor
{
    bool operator ()() const;
};

extern void init_fun();

struct init_ftor
{
    void operator()() const;
};

extern void step_fun();

struct step_ftor
{
    void operator()() const;
};

extern bool cmp_fun();

struct cmp_ftor
{
    bool operator()() const;
};

int
main()
{
    //////////////////////////////////
    // compile time check wether as_parser<> works for for_p

    ::BOOST_SPIRIT_CLASSIC_NS::rule<> r;

    r = BOOST_SPIRIT_CLASSIC_NS::for_p(&init_fun, &cmp_fun, &step_fun)['-'];
    r = BOOST_SPIRIT_CLASSIC_NS::for_p(init_ftor(), cmp_ftor(), step_ftor())["-"];

    r = BOOST_SPIRIT_CLASSIC_NS::for_p(init_ftor(), r, step_ftor())[r];
}
