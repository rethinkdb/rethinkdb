/*=============================================================================
    Copyright (c) 2004 Joao Abecasis
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/spirit/include/classic_primitives.hpp>
#include <boost/spirit/include/classic_rule.hpp>

#include <string>

int main()
{
    using BOOST_SPIRIT_CLASSIC_NS::rule;
    using BOOST_SPIRIT_CLASSIC_NS::str_p;
    using BOOST_SPIRIT_CLASSIC_NS::ch_p;

    using std::string;

    string str = "abcd";

    rule<> strings = str_p("abcd");
    strings = str_p('a');
    strings = str_p(str.begin(), str.end());

    rule<> chars = ch_p('a');
    chars = ch_p("b");
}

