/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/algorithm/query/count.hpp>
#include <boost/mpl/vector_c.hpp>
#include <string>

int
main()
{
    {
        boost::fusion::vector<int, short, double> t(1, 1, 1);
        BOOST_TEST(boost::fusion::count(t, 1) == 3);
    }

    {
        boost::fusion::vector<int, short, double> t(1, 2, 3.3);
        BOOST_TEST(boost::fusion::count(t, 3) == 0);
    }

    {
        boost::fusion::vector<int, std::string, double> t(4, "hello", 4);
        BOOST_TEST(boost::fusion::count(t, "hello") == 1);
    }

    {
        typedef boost::mpl::vector_c<int, 1, 2, 2, 2, 3, 3> mpl_vec;
        BOOST_TEST(boost::fusion::count(mpl_vec(), 2) == 3);
        BOOST_TEST(boost::fusion::count(mpl_vec(), 3) == 2);
    }

    return boost::report_errors();
}

