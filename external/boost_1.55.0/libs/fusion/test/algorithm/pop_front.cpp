/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/algorithm/transformation/pop_front.hpp>
#include <boost/mpl/vector_c.hpp>

int
main()
{
    using namespace boost::fusion;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

/// Testing pop_front

    {
        char const* s = "Ruby";
        typedef vector<int, char, double, char const*> vector_type;
        vector_type t1(1, 'x', 3.3, s);

        {
            std::cout << pop_front(t1) << std::endl;
            BOOST_TEST((pop_front(t1) == make_vector('x', 3.3, s)));
        }
    }

    {
        typedef boost::mpl::vector_c<int, 1, 2, 3, 4, 5> mpl_vec;
        std::cout << boost::fusion::pop_front(mpl_vec()) << std::endl;
        BOOST_TEST((boost::fusion::pop_front(mpl_vec()) == make_vector(2, 3, 4, 5)));
    }

    return boost::report_errors();
}

