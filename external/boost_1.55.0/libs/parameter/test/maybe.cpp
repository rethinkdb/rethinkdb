// Copyright Daniel Wallin 2006. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/parameter/keyword.hpp>
#include <boost/parameter/aux_/maybe.hpp>
#include <cassert>

namespace test {

BOOST_PARAMETER_KEYWORD(tag, kw)
BOOST_PARAMETER_KEYWORD(tag, unused)
    
template <class Args>
int f(Args const& args)
{
    return args[kw | 1.f];
}

} // namespace test

int main()
{
    using test::kw;
    using test::unused;
    using test::f;
    using boost::parameter::aux::maybe;

    assert(f((kw = 0, unused = 0)) == 0);
    assert(f(unused = 0) == 1);
    assert(f((kw = maybe<int>(), unused = 0)) == 1);
    assert(f((kw = maybe<int>(2), unused = 0)) == 2);
    return 0;
}

