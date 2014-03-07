/*=============================================================================
    Copyright (c) 2004 Joao Abecasis
    Copyright (c) 2004 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_dynamic.hpp>
#include <boost/spirit/include/phoenix1.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <iostream>
#include <string>

using namespace BOOST_SPIRIT_CLASSIC_NS;
using namespace phoenix;

int main()
{
    using std::cout;
    using std::endl;
    using std::string;

    bool f = false;

    rule<> start =
    while_p(~eps_p(anychar_p[var(f) = true]))
    [
        anychar_p
    ];

    parse("This goes to prove my point.", start);
    BOOST_TEST(f == false);
    return boost::report_errors();
}


