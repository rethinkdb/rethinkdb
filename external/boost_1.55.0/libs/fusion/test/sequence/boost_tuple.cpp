/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/adapted/boost_tuple.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/sequence/intrinsic/empty.hpp>
#include <boost/fusion/sequence/intrinsic/front.hpp>
#include <boost/fusion/sequence/intrinsic/back.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/iterator/distance.hpp> 
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/container/list/list.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/sequence/comparison/not_equal_to.hpp>
#include <boost/fusion/sequence/comparison/less.hpp>
#include <boost/fusion/sequence/comparison/less_equal.hpp>
#include <boost/fusion/sequence/comparison/greater.hpp>
#include <boost/fusion/sequence/comparison/greater_equal.hpp>
#include <boost/fusion/mpl.hpp>
#include <boost/fusion/support/is_view.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/assert.hpp>
#include <iostream>
#include <string>

int
main()
{
    using namespace boost::fusion;
    using namespace boost;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

    {
        typedef boost::tuple<int, std::string> tuple_type;
        BOOST_MPL_ASSERT_NOT((traits::is_view<tuple_type>));
        tuple_type t(123, "Hola!!!");

        std::cout << at_c<0>(t) << std::endl;
        std::cout << at_c<1>(t) << std::endl;
        std::cout << t << std::endl;
        BOOST_TEST(t == make_vector(123, "Hola!!!"));

        at_c<0>(t) = 6;
        at_c<1>(t) = "mama mia";
        BOOST_TEST(t == make_vector(6, "mama mia"));

        BOOST_STATIC_ASSERT(boost::fusion::result_of::size<tuple_type>::value == 2);
        BOOST_STATIC_ASSERT(!boost::fusion::result_of::empty<tuple_type>::value);

        BOOST_TEST(front(t) == 6);
    }
    
    {
        fusion::vector<int, float> v1(4, 3.3f);
        boost::tuple<short, float> v2(5, 3.3f);
        fusion::vector<long, double> v3(5, 4.4);
        BOOST_TEST(v1 < v2);
        BOOST_TEST(v1 <= v2);
        BOOST_TEST(v2 > v1);
        BOOST_TEST(v2 >= v1);
        BOOST_TEST(v2 < v3);
        BOOST_TEST(v2 <= v3);
        BOOST_TEST(v3 > v2);
        BOOST_TEST(v3 >= v2);
    }

    {
        // conversion from boost tuple to vector
        fusion::vector<int, std::string> v(tuples::make_tuple(123, "Hola!!!"));
        v = tuples::make_tuple(123, "Hola!!!");
    }

    {
        // conversion from boost tuple to list
        fusion::list<int, std::string> l(tuples::make_tuple(123, "Hola!!!"));
        l = tuples::make_tuple(123, "Hola!!!");
    }
    
    { 
        // test from Ticket #1601, submitted by Shunsuke Sogame 
        // expanded by Stjepan Rajko 
        boost::tuple<int, char> t(3, 'a'); 

        BOOST_TEST(0u == fusion::distance(fusion::begin(t), fusion::begin(t))); 
        BOOST_TEST(1u == fusion::distance(fusion::begin(t), fusion::next(fusion::begin(t)))); 
        BOOST_TEST(2u == fusion::distance(fusion::begin(t), fusion::end(t))); 
    } 

    {
        typedef boost::tuple<int, std::string> tuple_type;
        BOOST_MPL_ASSERT((mpl::is_sequence<tuple_type>));
        BOOST_MPL_ASSERT((boost::is_same<int, mpl::front<tuple_type>::type>));
    }

    return boost::report_errors();
}

