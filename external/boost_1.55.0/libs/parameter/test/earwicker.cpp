// Copyright David Abrahams, Daniel Wallin 2005. Use, modification and 
// distribution is subject to the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/parameter.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/mpl/placeholders.hpp>
#include <iostream>

namespace test {

BOOST_PARAMETER_KEYWORD(tag, x)
BOOST_PARAMETER_KEYWORD(tag, y)
BOOST_PARAMETER_KEYWORD(tag, z)

using namespace boost::parameter;
using namespace boost::mpl::placeholders;

struct f_parameters // vc6 is happier with inheritance than with a typedef
  : parameters<
        optional<tag::x,boost::is_convertible<_,int> >
      , optional<tag::y,boost::is_convertible<_,int> >
      , optional<tag::z,boost::is_convertible<_,int> >
    >
{};

#ifdef BOOST_NO_VOID_RETURNS
BOOST_PARAMETER_FUN(int, f, 0, 3, f_parameters)
#else 
BOOST_PARAMETER_FUN(void, f, 0, 3, f_parameters)
#endif 
{
    std::cout << "x = " << p[x | -1] << std::endl;
    std::cout << "y = " << p[y | -2] << std::endl;
    std::cout << "z = " << p[z | -3] << std::endl;
    std::cout <<  "================" << std::endl;
#ifdef BOOST_NO_VOID_RETURNS
    return 0;
#endif 
}

}


int main()
{
    using namespace test;
    f(x = 1, y = 2, z = 3);
    f(x = 1);
    f(y = 2);
    f(z = 3);
    f(z = 3, x = 1);
}


