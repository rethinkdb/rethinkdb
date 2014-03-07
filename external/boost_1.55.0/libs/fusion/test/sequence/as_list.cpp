/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/container/generation/make_list.hpp>
#include <boost/fusion/container/list/convert.hpp>
#include <boost/fusion/algorithm/transformation/push_back.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/mpl/vector_c.hpp>

int
main()
{
    using namespace boost::fusion;
    using namespace boost;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

    {
        vector0<> empty;
        std::cout << as_list(make_vector(1, 1.23, "harru")) << std::endl; 
        std::cout << as_list(push_back(empty, 999)) << std::endl;

        BOOST_TEST(as_list(make_vector(1, 1.23, "harru")) == make_vector(1, 1.23, std::string("harru")));
        BOOST_TEST(as_list(push_back(empty, 999)) == push_back(empty, 999));
    }

    {
        std::cout << as_list(mpl::vector_c<int, 1, 2, 3, 4, 5>()) << std::endl;
        BOOST_TEST((as_list(mpl::vector_c<int, 1, 2, 3, 4, 5>()) 
            == mpl::vector_c<int, 1, 2, 3, 4, 5>()));
    }

    {
        // test conversion
        list<int, std::string> l(make_vector(123, "harru"));
        BOOST_TEST(l == make_vector(123, "harru"));
        l = (make_vector(235, "hola")); // test assign
        BOOST_TEST(l == make_vector(235, "hola"));
    }

    return boost::report_errors();
}

