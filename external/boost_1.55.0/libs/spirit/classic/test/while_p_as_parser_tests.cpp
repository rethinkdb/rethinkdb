/*=============================================================================
    Copyright (c) 2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_while.hpp>

extern bool fun();

struct ftor
{
    bool operator()() const;
};

int
main()
{
    //////////////////////////////////
    // compile time check wether as_parser<> works for while_p

    ::BOOST_SPIRIT_CLASSIC_NS::rule<> r;

    r = ::BOOST_SPIRIT_CLASSIC_NS::while_p('-')['-'];
    r = ::BOOST_SPIRIT_CLASSIC_NS::while_p("-")["-"];

    r = ::BOOST_SPIRIT_CLASSIC_NS::while_p(&fun)["foo"];
    r = ::BOOST_SPIRIT_CLASSIC_NS::while_p(ftor())["foo"];

    r = ::BOOST_SPIRIT_CLASSIC_NS::while_p(r)[r];

    r = ::BOOST_SPIRIT_CLASSIC_NS::do_p['-'].while_p('-');
    r = ::BOOST_SPIRIT_CLASSIC_NS::do_p["-"].while_p("-");

    r = ::BOOST_SPIRIT_CLASSIC_NS::do_p["foo"].while_p(&fun);
    r = ::BOOST_SPIRIT_CLASSIC_NS::do_p["foo"].while_p(ftor());

    r = ::BOOST_SPIRIT_CLASSIC_NS::do_p[r].while_p(r);
}
