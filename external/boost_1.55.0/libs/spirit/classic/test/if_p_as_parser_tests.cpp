/*=============================================================================
    Copyright (c) 2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_if.hpp>

extern bool fun();

struct ftor
{
    bool operator()() const;
};

int
main()
{
    //////////////////////////////////
    // compile time check wether as_parser<> works for if_p

    ::BOOST_SPIRIT_CLASSIC_NS::rule<> r;

    r = ::BOOST_SPIRIT_CLASSIC_NS::if_p('-')['-'];
    r = ::BOOST_SPIRIT_CLASSIC_NS::if_p("-")["-"];
    r = ::BOOST_SPIRIT_CLASSIC_NS::if_p('-')['-'].else_p['-'];
    r = ::BOOST_SPIRIT_CLASSIC_NS::if_p("-")["-"].else_p["-"];
    
    r = ::BOOST_SPIRIT_CLASSIC_NS::if_p(&fun)["foo"];
    r = ::BOOST_SPIRIT_CLASSIC_NS::if_p(ftor())["foo"];
    r = ::BOOST_SPIRIT_CLASSIC_NS::if_p(&fun)["foo"].else_p["bar"];
    r = ::BOOST_SPIRIT_CLASSIC_NS::if_p(ftor())["foo"].else_p["bar"];

    r = ::BOOST_SPIRIT_CLASSIC_NS::if_p(r)[r];
    r = ::BOOST_SPIRIT_CLASSIC_NS::if_p(r)[r].else_p[r];
}
