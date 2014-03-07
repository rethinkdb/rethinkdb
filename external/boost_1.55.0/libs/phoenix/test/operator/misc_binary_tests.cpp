/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <string>
#include <boost/detail/lightweight_test.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>

namespace phoenix = boost::phoenix;

int
main()
{
    using phoenix::ref;
    using phoenix::val;
    using phoenix::arg_names::arg1;
    using phoenix::arg_names::arg2;
    using std::string;
    {   // From Phoenix 1.1 binary tests

        int i2 = 2, i3 = 3, i = 5;
        const char* world = " world";


        BOOST_TEST((ref(i) = ref(i))() == 5);
        BOOST_TEST((ref(i) = 3)() == 3);
        BOOST_TEST(i == 3);
        i = 5;
        int x, y, z;
        (ref(x) = ref(y) = ref(z) = 10)();
        BOOST_TEST(x == 10 && y == 10 && z == 10);
        BOOST_TEST((val(world)[3])() == world[3]);

        BOOST_TEST((ref(i) += 5)() == 10);
        BOOST_TEST((ref(i) -= 5)() == 5);
        BOOST_TEST((ref(i) *= 5)() == 25);
        BOOST_TEST((ref(i) /= 5)() == 5);
        BOOST_TEST((ref(i) %= 2)() == 1);

        BOOST_TEST((ref(i) <<= 3)() == 8);
        BOOST_TEST((ref(i) >>= 1)() == 4);
        BOOST_TEST((ref(i) |= 0xFF)() == 0xFF);
        BOOST_TEST((ref(i) &= 0xF0)() == 0xF0);
        BOOST_TEST((ref(i) ^= 0xFFFFFFFF)() == int(0xFFFFFF0F));

        BOOST_TEST((val(5) == val(5))());
        BOOST_TEST((val(5) == 5)());

        BOOST_TEST((arg1 + arg2 )(i2, i3) == i2 + i3 );
        BOOST_TEST((arg1 - arg2)(i2, i3) == i2 - i3);
        BOOST_TEST((arg1 * arg2)(i2, i3) == i2 * i3);
        BOOST_TEST((arg1 / arg2)(i2, i3) == i2 / i3);
        BOOST_TEST((arg1 % arg2)(i2, i3) == i2 % i3);
        BOOST_TEST((arg1 & arg2)(i2, i3) == (i2 & i3));
        BOOST_TEST((arg1 | arg2)(i2, i3) == (i2 | i3));
        BOOST_TEST((arg1 ^ arg2)(i2, i3) == (i2 ^ i3));
        BOOST_TEST((arg1 << arg2)(i2, i3) == i2 << i3);
        BOOST_TEST((arg1 >> arg2)(i2, i3) == i2 >> i3);

        BOOST_TEST((val(5) != val(6))());
        BOOST_TEST((val(5) < val(6))());
        BOOST_TEST(!(val(5) > val(6))());
        BOOST_TEST((val(5) < val(6))());
        BOOST_TEST((val(5) <= val(6))());
        BOOST_TEST((val(5) <= val(5))());
        BOOST_TEST((val(7) >= val(6))());
        BOOST_TEST((val(7) >= val(7))());

        BOOST_TEST((val(false) && val(false))() == false);
        BOOST_TEST((val(true) && val(false))() == false);
        BOOST_TEST((val(false) && val(true))() == false);
        BOOST_TEST((val(true) && val(true))() == true);

        BOOST_TEST((val(false) || val(false))() == false);
        BOOST_TEST((val(true) || val(false))() == true);
        BOOST_TEST((val(false) || val(true))() == true);
        BOOST_TEST((val(true) || val(true))() == true);
    }

    {  // From Phoenix 1.1 mixed_binary tests

        int     i1 = 1, i2 = 2, i50 = 50, i100 = 100;
        double  d2_5 = 2.5;
        string hello = "hello";
        const char* world = " world";

        BOOST_TEST((arg1 + arg2)(i100, i50) == (i100 + i50));
        BOOST_TEST((arg1 + 3)(i100) == (3 + i100));
        BOOST_TEST((arg1 + arg2)(hello, world) == "hello world");
        BOOST_TEST((arg1 + arg2)(i1, d2_5) == (i1 + d2_5));

        BOOST_TEST((*(arg1 + arg2))(world, i2) == *(world + i2));
        BOOST_TEST((*(arg1 + arg2))(i2, world) == *(i2 + world));
        BOOST_TEST((*(val(world+i2) - arg1))(i2) == *world);
    }

    return boost::report_errors();
}
