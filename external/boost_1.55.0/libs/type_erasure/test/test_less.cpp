// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_less.cpp 83251 2013-03-02 19:23:44Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/tuple.hpp>
#include <boost/mpl/vector.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::type_erasure;

BOOST_AUTO_TEST_CASE(test_basic)
{
    typedef boost::mpl::vector<copy_constructible<>, less_than_comparable<> > test_concept;
    any<test_concept> x(1);
    any<test_concept> y(2);

    BOOST_CHECK((x < y));
    BOOST_CHECK(!(y < x));
    BOOST_CHECK(!(x < x));

    BOOST_CHECK(!(x > y));
    BOOST_CHECK((y > x));
    BOOST_CHECK(!(x > x));

    BOOST_CHECK((x <= y));
    BOOST_CHECK(!(y <= x));
    BOOST_CHECK((x <= x));
    
    BOOST_CHECK(!(x >= y));
    BOOST_CHECK((y >= x));
    BOOST_CHECK((x >= x));
}

BOOST_AUTO_TEST_CASE(test_mixed_less)
{
    typedef boost::mpl::vector<copy_constructible<_a>, copy_constructible<_b>, less_than_comparable<_a, _b> > test_concept;
    tuple<test_concept, _a, _b> t(1, 2.0);
    any<test_concept, _a> x(get<0>(t));
    any<test_concept, _b> y(get<1>(t));

    BOOST_CHECK((x < y));
    BOOST_CHECK((y > x));
    BOOST_CHECK(!(y <= x));
    BOOST_CHECK(!(x >= y));
}

BOOST_AUTO_TEST_CASE(test_mixed_equal)
{
    typedef boost::mpl::vector<copy_constructible<_a>, copy_constructible<_b>, less_than_comparable<_a, _b> > test_concept;
    tuple<test_concept, _a, _b> t(1, 1);
    any<test_concept, _a> x(get<0>(t));
    any<test_concept, _b> y(get<1>(t));

    BOOST_CHECK(!(x < y));
    BOOST_CHECK(!(y > x));
    BOOST_CHECK((y <= x));
    BOOST_CHECK((x >= y));
}

BOOST_AUTO_TEST_CASE(test_mixed_greater)
{
    typedef boost::mpl::vector<copy_constructible<_a>, copy_constructible<_b>, less_than_comparable<_a, _b> > test_concept;
    tuple<test_concept, _a, _b> t(2.0, 1);
    any<test_concept, _a> x(get<0>(t));
    any<test_concept, _b> y(get<1>(t));

    BOOST_CHECK(!(x < y));
    BOOST_CHECK(!(y > x));
    BOOST_CHECK((y <= x));
    BOOST_CHECK((x >= y));
}

BOOST_AUTO_TEST_CASE(test_fixed_lhs_less)
{
    typedef boost::mpl::vector<copy_constructible<>, less_than_comparable<int, _self> > test_concept;
    int x(1);
    any<test_concept> y(2.0);

    BOOST_CHECK((x < y));
    BOOST_CHECK((y > x));
    BOOST_CHECK(!(y <= x));
    BOOST_CHECK(!(x >= y));
}

BOOST_AUTO_TEST_CASE(test_fixed_lhs_equal)
{
    typedef boost::mpl::vector<copy_constructible<>, less_than_comparable<int, _self> > test_concept;
    int x(1);
    any<test_concept> y(1);

    BOOST_CHECK(!(x < y));
    BOOST_CHECK(!(y > x));
    BOOST_CHECK((y <= x));
    BOOST_CHECK((x >= y));
}


BOOST_AUTO_TEST_CASE(test_fixed_lhs_greater)
{
    typedef boost::mpl::vector<copy_constructible<>, less_than_comparable<int, _self> > test_concept;
    int x(1);
    any<test_concept> y(0.5);

    BOOST_CHECK(!(x < y));
    BOOST_CHECK(!(y > x));
    BOOST_CHECK((y <= x));
    BOOST_CHECK((x >= y));
}

BOOST_AUTO_TEST_CASE(test_fixed_rhs_less)
{
    typedef boost::mpl::vector<copy_constructible<>, less_than_comparable<_self, int> > test_concept;
    any<test_concept> x(1.0);
    int y(2);

    BOOST_CHECK((x < y));
    BOOST_CHECK((y > x));
    BOOST_CHECK(!(y <= x));
    BOOST_CHECK(!(x >= y));
}

BOOST_AUTO_TEST_CASE(test_fixed_rhs_equal)
{
    typedef boost::mpl::vector<copy_constructible<>, less_than_comparable<_self, int> > test_concept;
    any<test_concept> x(1);
    int y(1);

    BOOST_CHECK(!(x < y));
    BOOST_CHECK(!(y > x));
    BOOST_CHECK((y <= x));
    BOOST_CHECK((x >= y));
}


BOOST_AUTO_TEST_CASE(test_fixed_rhs_greater)
{
    typedef boost::mpl::vector<copy_constructible<>, less_than_comparable<_self, int> > test_concept;
    any<test_concept> x(2.0);
    int y(1);

    BOOST_CHECK(!(x < y));
    BOOST_CHECK(!(y > x));
    BOOST_CHECK((y <= x));
    BOOST_CHECK((x >= y));
}

BOOST_AUTO_TEST_CASE(test_relaxed)
{
    typedef boost::mpl::vector<copy_constructible<>, less_than_comparable<>, relaxed> test_concept;
    any<test_concept> x(1);
    any<test_concept> y(2);
    any<test_concept> z(std::string("test"));

    BOOST_CHECK((x < y));
    BOOST_CHECK(!(y < x));
    BOOST_CHECK(!(x < x));

    BOOST_CHECK(!(x > y));
    BOOST_CHECK((y > x));
    BOOST_CHECK(!(x > x));

    BOOST_CHECK((x <= y));
    BOOST_CHECK(!(y <= x));
    BOOST_CHECK((x <= x));
    
    BOOST_CHECK(!(x >= y));
    BOOST_CHECK((y >= x));
    BOOST_CHECK((x >= x));

    bool expected = x < z;

    BOOST_CHECK_EQUAL((x < z), expected);
    BOOST_CHECK_EQUAL(!(z < x), expected);

    BOOST_CHECK_EQUAL(!(x > z), expected);
    BOOST_CHECK_EQUAL((z > x), expected);

    BOOST_CHECK_EQUAL((x <= z), expected);
    BOOST_CHECK_EQUAL(!(z <= x), expected);
    
    BOOST_CHECK_EQUAL(!(x >= z), expected);
    BOOST_CHECK_EQUAL((z >= x), expected);
    
    BOOST_CHECK_EQUAL((y < z), expected);
    BOOST_CHECK_EQUAL(!(z < y), expected);

    BOOST_CHECK_EQUAL(!(y > z), expected);
    BOOST_CHECK_EQUAL((z > y), expected);

    BOOST_CHECK_EQUAL((y <= z), expected);
    BOOST_CHECK_EQUAL(!(z <= y), expected);
    
    BOOST_CHECK_EQUAL(!(y >= z), expected);
    BOOST_CHECK_EQUAL((z >= y), expected);
}

BOOST_AUTO_TEST_CASE(test_overload)
{
    typedef boost::mpl::vector<
        copy_constructible<_a>,
        copy_constructible<_b>,
        less_than_comparable<_a>,
        less_than_comparable<_a, int>,
        less_than_comparable<int, _a>,
        less_than_comparable<_b>,
        less_than_comparable<_b, int>,
        less_than_comparable<int, _b>,
        less_than_comparable<_a, _b>
    > test_concept;
    tuple<test_concept, _a, _b> t(1, 2);
    any<test_concept, _a> x(get<0>(t));
    any<test_concept, _b> y(get<1>(t));

    BOOST_CHECK(!(x < x));
    BOOST_CHECK(x <= x);
    BOOST_CHECK(!(x > x));
    BOOST_CHECK(x >= x);
    
    BOOST_CHECK(!(x < 1));
    BOOST_CHECK(x <= 1);
    BOOST_CHECK(!(x > 1));
    BOOST_CHECK(x >= 1);

    BOOST_CHECK(!(1 < x));
    BOOST_CHECK(1 <= x);
    BOOST_CHECK(!(1 > x));
    BOOST_CHECK(1 >= x);
    
    BOOST_CHECK(!(y < y));
    BOOST_CHECK(y <= y);
    BOOST_CHECK(!(y > y));
    BOOST_CHECK(y >= y);
    
    BOOST_CHECK(!(y < 2));
    BOOST_CHECK(y <= 2);
    BOOST_CHECK(!(y > 2));
    BOOST_CHECK(y >= 2);

    BOOST_CHECK(!(2 < y));
    BOOST_CHECK(2 <= y);
    BOOST_CHECK(!(2 > y));
    BOOST_CHECK(2 >= y);
    
    BOOST_CHECK(x < y);
    BOOST_CHECK(y > x);
    BOOST_CHECK(!(y <= x));
    BOOST_CHECK(!(x >= y));
}
