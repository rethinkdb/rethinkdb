/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/algorithm/query/count_if.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/mpl/vector_c.hpp>
#include <functional>

int
main()
{
    {
        boost::fusion::vector<int, short, double> t(1, 2, 3.3);
        BOOST_TEST(boost::fusion::count_if(t, boost::lambda::_1 == 2) == 1);
    }

    {
        boost::fusion::vector<int, short, double> t(1, 2, 3.3);
        BOOST_TEST(boost::fusion::count_if(t, boost::lambda::_1 == 3) == 0);
    }

    {
        typedef boost::mpl::vector_c<int, 1, 2, 3> mpl_vec;
        // Cannot use lambda here as mpl iterators return rvalues and lambda needs lvalues
        BOOST_TEST(boost::fusion::count_if(mpl_vec(), std::bind2nd(std::less_equal<int>(), 2)) == 2);
        BOOST_TEST(boost::fusion::count_if(mpl_vec(), std::bind2nd(std::greater<int>(), 2)) == 1);
    }

    return boost::report_errors();
}

