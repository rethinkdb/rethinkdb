/*=============================================================================
    Copyright (c) 1999-2003 Jaakko Jarvi
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/sequence/comparison.hpp>

void
equality_test()
{
    using namespace boost::fusion;

    FUSION_SEQUENCE<int, char> v1(5, 'a');
    FUSION_SEQUENCE<int, char> v2(5, 'a');
    BOOST_TEST(v1 == v2);

    FUSION_SEQUENCE<int, char> v3(5, 'b');
    FUSION_SEQUENCE<int, char> t4(2, 'a');
    BOOST_TEST(v1 != v3);
    BOOST_TEST(v1 != t4);
    BOOST_TEST(!(v1 != v2));

    FUSION_SEQUENCE<int, char, bool> v5(5, 'a', true);
    BOOST_TEST(v1 != v5);
    BOOST_TEST(!(v1 == v5));
    BOOST_TEST(v5 != v1);
    BOOST_TEST(!(v5 == v1));
}

void
ordering_test()
{
    using namespace boost::fusion;

    FUSION_SEQUENCE<int, float> v1(4, 3.3f);
    FUSION_SEQUENCE<short, float> v2(5, 3.3f);
    FUSION_SEQUENCE<long, double> v3(5, 4.4);
    BOOST_TEST(v1 < v2);
    BOOST_TEST(v1 <= v2);
    BOOST_TEST(v2 > v1);
    BOOST_TEST(v2 >= v1);
    BOOST_TEST(v2 < v3);
    BOOST_TEST(v2 <= v3);
    BOOST_TEST(v3 > v2);
    BOOST_TEST(v3 >= v2);

#if defined(FUSION_TEST_FAIL)
    FUSION_SEQUENCE<int, char, bool> v5(5, 'a', true);
    v1 >= v5;
#endif
}



