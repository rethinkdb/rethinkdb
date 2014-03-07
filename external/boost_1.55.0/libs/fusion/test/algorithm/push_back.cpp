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
#include <boost/fusion/algorithm/transformation/push_back.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/mpl/vector_c.hpp>
#include <string>

struct plus_one
{
    template <typename T>
    void operator()(T& v) const
    {
        v += 1;
    }
};

int
main()
{
    using namespace boost::fusion;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

/// Testing push_back

    {
        char const* s = "Ruby";
        typedef vector<int, char, double, char const*> vector_type;
        vector_type t1(1, 'x', 3.3, s);

        {
            std::cout << push_back(t1, 123456) << std::endl;
            BOOST_TEST((push_back(t1, 123456)
                == make_vector(1, 'x', 3.3, s, 123456)));
        }

        {
            std::cout << push_back(t1, "funny") << std::endl;
            BOOST_TEST((push_back(t1, "funny")
                == make_vector(1, 'x', 3.3, s, std::string("funny"))));
        }

        {
            std::cout << push_back(t1, t1) << std::endl;
            BOOST_TEST((push_back(t1, t1)
                == make_vector(1, 'x', 3.3, s, t1)));
        }
    }
    
    {
        typedef boost::mpl::vector_c<int, 1, 2, 3, 4, 5> mpl_vec;
        std::cout << boost::fusion::push_back(mpl_vec(), boost::mpl::int_<6>()) << std::endl;
        BOOST_TEST((boost::fusion::push_back(mpl_vec(), boost::mpl::int_<6>())
            == make_vector(1, 2, 3, 4, 5, 6)));
    }

    return boost::report_errors();
}

