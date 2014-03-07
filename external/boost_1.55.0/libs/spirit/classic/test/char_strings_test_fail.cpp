/*=============================================================================
    Copyright (c) 2004 Joao Abecasis
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/spirit/include/classic_primitives.hpp>
#include <boost/spirit/include/classic_rule.hpp>

int main()
{
    using BOOST_SPIRIT_CLASSIC_NS::rule;
    using BOOST_SPIRIT_CLASSIC_NS::ch_p;

    rule<> chars = ch_p("string");
}
