// Copyright Daniel Wallin 2005. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/parameter/keyword.hpp>
#include <boost/detail/lightweight_test.hpp>

BOOST_PARAMETER_KEYWORD(tag, x)
BOOST_PARAMETER_KEYWORD(tag, y)
    
struct default_src
{
    typedef int result_type;

    int operator()() const
    {
        return 0;
    }
};
    
template <class ArgumentPack, class K, class T>
void check(ArgumentPack const& p, K const& kw, T const& value)
{
    BOOST_TEST(p[kw] == value);
}

int main()
{
    check(x = 20, x, 20);
    check(y = 20, y, 20);

    check(x = 20, x | 0, 20);
    check(y = 20, y | 0, 20);

    check(x = 20, x | default_src(), 20);
    check(y = 20, y | default_src(), 20);
    
    check(y = 20, x | 0, 0);
    check(y = 20, x || default_src(), 0);
    return boost::report_errors();
}

