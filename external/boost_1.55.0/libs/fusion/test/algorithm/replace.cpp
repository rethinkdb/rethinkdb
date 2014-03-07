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
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/algorithm/transformation/replace.hpp>
#include <string>

int
main()
{
    using namespace boost::fusion;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

/// Testing replace

    {
        char const* s = "Ruby";
        typedef vector<int, char, long, char const*> vector_type;
        vector_type t1(1, 'x', 3, s);

        {
            std::cout << replace(t1, 'x', 'y') << std::endl;
            BOOST_TEST((replace(t1, 'x', 'y')
                == make_vector(1, 'y', 3, s)));
        }

        {
            char const* s2 = "funny";
            std::cout << replace(t1, s, s2) << std::endl;
            BOOST_TEST((replace(t1, s, s2)
                == make_vector(1, 'x', 3, s2)));
        }
    }

    return boost::report_errors();
}

