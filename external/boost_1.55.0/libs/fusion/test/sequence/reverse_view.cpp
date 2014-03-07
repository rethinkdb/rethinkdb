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
#include <boost/fusion/view/reverse_view/reverse_view.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/sequence/intrinsic/value_at.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/fusion/iterator/prior.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/iterator/advance.hpp>
#include <boost/fusion/iterator/distance.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/type_traits/is_same.hpp>


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
        mpl_list1 l;
        reverse_view<mpl_list1> rev(l);

        std::cout << rev << std::endl;
        BOOST_TEST((rev == make_vector(8, 7, 6, 5)));
    }

    {
        char const* s = "Hi Kim";
        typedef vector<int, char, long, char const*> vector_type;
        vector_type t(123, 'x', 123456789, s);
        typedef reverse_view<vector_type> view_type;
        view_type rev(t);

        std::cout << rev << std::endl;
        BOOST_TEST((rev == make_vector(s, 123456789, 'x', 123)));

        typedef boost::fusion::result_of::begin<view_type>::type first_type;
        first_type first_it(begin(rev));
        typedef boost::fusion::result_of::next<first_type>::type second_type;
        second_type second_it(next(first_it));
        BOOST_TEST((*second_it == 123456789));
        BOOST_TEST((*prior(second_it) == s));
        BOOST_TEST((*advance_c<2>(first_it) == 'x'));
        BOOST_TEST((distance(first_it, second_it) == 1));

        BOOST_TEST((at_c<0>(rev)==s));
        BOOST_TEST((at_c<1>(rev)==123456789));
        BOOST_TEST((at_c<2>(rev)=='x'));
        BOOST_TEST((at_c<3>(rev)==123));

        BOOST_MPL_ASSERT((
            boost::is_same<boost::fusion::result_of::value_at_c<view_type,0>::type,char const*>
        ));
        BOOST_MPL_ASSERT((
            boost::is_same<boost::fusion::result_of::value_at_c<view_type,1>::type,long>
        ));
        BOOST_MPL_ASSERT((
            boost::is_same<boost::fusion::result_of::value_at_c<view_type,2>::type,char>
        ));
        BOOST_MPL_ASSERT((
            boost::is_same<boost::fusion::result_of::value_at_c<view_type,3>::type,int>
        ));
    }

    return boost::report_errors();
}

