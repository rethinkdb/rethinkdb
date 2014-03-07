/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <string>
#include <cmath>

#include <boost/detail/lightweight_test.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/scope/dynamic.hpp>

struct my_dynamic : ::boost::phoenix::dynamic<int, std::string, double>
{
    my_dynamic() : num(init<0>(this)), message(init<1>(this)), real(init<2>(this)) {}

    member1 num;
    member2 message;
    member3 real;
};

//  You may also use the macro below to achieve the same as above:
//
//BOOST_PHOENIX_DYNAMIC(
//    my_dynamic,
//        (int, num)
//        (std::string, message)
//        (double, real)
//);

int
main()
{
    using namespace boost::phoenix;
    using namespace boost::phoenix::arg_names;

    my_dynamic clos;

    {   //  First stack frame
        dynamic_frame<my_dynamic::self_type> frame(clos);
        (clos.num = 123)();
        (clos.num += 456)();
        (clos.real = clos.num / 56.5)();
        (clos.message = "Hello " + std::string("World "))();

        {   //  Second stack frame
            dynamic_frame<my_dynamic::self_type> frame(clos);
            (clos.num = 987)();
            (clos.message = std::string("Abracadabra "))();
            (clos.real = clos.num * 1e30)();

            {   //  Third stack frame
                boost::phoenix::vector3<int, std::string, double> init = {-1, "Direct Init ", 3.14};
                dynamic_frame<my_dynamic::self_type> frame(clos, init);

                (std::cout << clos.message << clos.num << ", " << clos.real << '\n')();
                BOOST_TEST(clos.num() == -1);
                BOOST_TEST(clos.message() == "Direct Init ");
            }

            (std::cout << clos.message << clos.num << ", " << clos.real << '\n')();
            BOOST_TEST(clos.num() == 987);
            BOOST_TEST(clos.message() == "Abracadabra ");
        }

        (std::cout << clos.message << clos.num << ", " << clos.real << '\n')();
        BOOST_TEST(clos.num() == 123+456);
        BOOST_TEST(clos.message() == "Hello " + std::string("World "));
    }

    return boost::report_errors();
}
