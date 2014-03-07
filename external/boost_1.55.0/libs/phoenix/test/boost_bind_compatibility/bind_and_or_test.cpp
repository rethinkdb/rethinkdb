/*==============================================================================
    Copyright (c) 2008 Peter Dimov
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/bind.hpp>
#include <boost/phoenix/operator.hpp>

#include <iostream>
#include <boost/detail/lightweight_test.hpp>

bool
f(bool x)
{
    return x;
}

bool
g(bool x)
{
    return !x;
}

bool
h()
{
    BOOST_ERROR("Short-circuit evaluation failure");
    return false;
}

template <typename F, typename A1, typename A2, typename R>
void
tester(F f, A1 a1, A2 a2, R r)
{
    BOOST_TEST(f(a1, a2) == r);
}

int main()
{
    using boost::phoenix::bind;
    using boost::phoenix::placeholders::_1;
    using boost::phoenix::placeholders::_2;

    // &&

    tester(bind(f, true) && bind(g, true), false, false, f(true) && g(true));
    tester(bind(f, true) && bind(g, false), false, false, f(true) && g(false));

    tester(bind(f, false) && bind(h), false, false, f(false) && h());

    tester(bind(f, _1) && bind(g, _2), true, true, f(true) && g(true));
    tester(bind(f, _1) && bind(g, _2), true, false, f(true) && g(false));

    tester(bind(f, _1) && bind(h), false, false, f(false) && h());

    // ||

    tester(bind(f, false) || bind(g, true), false, false, f(false) || g(true));

    tester(bind(f, false) || bind(g, false), false, false, f(false) || g(false));

    tester(bind(f, true) || bind(h), false, false, f(true) || h());

    tester(bind(f, _1) || bind(g, _2), false, true, f(false) || g(true));
    tester(bind(f, _1) || bind(g, _2), false, false, f(false) || g(false));

    tester(bind(f, _1) || bind(h), true, false, f(true) || h());

    //

    return boost::report_errors();
}
