/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/algorithm/transformation/reverse.hpp>
#include <boost/mpl/range_c.hpp>

int
main()
{
    using namespace boost::fusion;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

/// Testing the reverse_view

    {
        typedef boost::mpl::range_c<int, 5, 9> mpl_list1;
        mpl_list1 sequence;

        std::cout << reverse(sequence) << std::endl;
        BOOST_TEST((reverse(sequence) == make_vector(8, 7, 6, 5)));
    }

    {
        char const* s = "Hi Kim";
        typedef vector<int, char, double, char const*> vector_type;
        vector_type t(123, 'x', 3.36, s);

        std::cout << reverse(t) << std::endl;
        BOOST_TEST((reverse(t) == make_vector(s, 3.36, 'x', 123)));
        std::cout << reverse(reverse(t)) << std::endl;
        BOOST_TEST((reverse(reverse(t)) == t));
    }

    return boost::report_errors();
}


