/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/container/list/list.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/container/generation/make_list.hpp>
#include <boost/fusion/algorithm/transformation/pop_back.hpp>
#include <boost/fusion/algorithm/transformation/push_back.hpp>
#include <boost/fusion/algorithm/query/find.hpp>
#include <boost/fusion/include/back.hpp>
#include <boost/fusion/include/array.hpp>
#include <boost/array.hpp>
#include <boost/mpl/vector_c.hpp>

int
main()
{
    using namespace boost::fusion;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

/// Testing pop_back

    {
        char const* s = "Ruby";
        typedef vector<int, char, double, char const*> vector_type;
        vector_type t1(1, 'x', 3.3, s);

        {
            std::cout << pop_back(t1) << std::endl;
            BOOST_TEST((pop_back(t1) == make_vector(1, 'x', 3.3)));
        }
    }

    {
        typedef boost::mpl::vector_c<int, 1, 2, 3, 4, 5> mpl_vec;
        std::cout << boost::fusion::pop_back(mpl_vec()) << std::endl;
        BOOST_TEST((boost::fusion::pop_back(mpl_vec()) == make_vector(1, 2, 3, 4)));
    }

    {
        list<int, int> l(1, 2);
        std::cout << pop_back(l) << std::endl;
        BOOST_TEST((pop_back(l) == make_list(1)));
    }

    { // make sure empty sequences are OK
        list<int> l(1);
        std::cout << pop_back(l) << std::endl;
        BOOST_TEST((pop_back(l) == make_list()));
    }

    {
        single_view<int> sv(1);
        std::cout << pop_back(sv) << std::endl;

        // Compile check only
        begin(pop_back(sv)) == end(sv);
        end(pop_back(sv)) == begin(sv);
    }

    // $$$ JDG: TODO add compile fail facility $$$
    //~ { // compile fail check (Disabled for now)
        //~ list<> l;
        //~ std::cout << pop_back(l) << std::endl;
    //~ }

#ifndef BOOST_NO_AUTO_DECLARATIONS
    {
        auto vec = make_vector(1, 3.14, "hello");

        // Compile check only
        auto popv = pop_back(vec);
        std::cout << popv << std::endl;

        auto push = push_back(vec, 42);
        auto pop = pop_back(vec);
        auto i1 = find<int>(popv);
        auto i2 = find<double>(pop);

        BOOST_TEST(i1 != end(pop));
        BOOST_TEST(i2 != end(pop));
        BOOST_TEST(i1 != i2);
    }
#endif

    {
        boost::array<std::size_t, 2> a = { 10, 50 };
        BOOST_TEST(back(pop_back(a)) == 10);
    }

    return boost::report_errors();
}

