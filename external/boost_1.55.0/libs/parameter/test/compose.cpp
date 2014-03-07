//~ Copyright Rene Rivera 2006.
//~ Use, modification and distribution is subject to the Boost Software License,
//~ Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at 
//~ http://www.boost.org/LICENSE_1_0.txt)

#include <boost/parameter.hpp>

namespace param
{
    BOOST_PARAMETER_KEYWORD(Tag,a0)
    BOOST_PARAMETER_KEYWORD(Tag,a1)
    BOOST_PARAMETER_KEYWORD(Tag,a2)
}

namespace test
{
    struct A
    {
        int i;
        int j;
        
        template <typename ArgPack> A(ArgPack const & args)
        {
            i = args[param::a0];
            j = args[param::a1];
        }
    };

    struct B : A
    {
        template <typename ArgPack> B(ArgPack const & args)
            : A((args, param::a0 = 1))
        {
        }
    };
}

int main()
{
    test::A a((param::a0 = 1, param::a1 = 13, param::a2 = 6));
    test::B b0((param::a1 = 13));
    test::B b1((param::a1 = 13, param::a2 = 6));
}
