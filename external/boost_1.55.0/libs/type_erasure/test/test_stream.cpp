// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_stream.cpp 78429 2012-05-12 02:37:24Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/tuple.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/tuple.hpp>
#include <boost/mpl/vector.hpp>
#include <sstream>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::type_erasure;

BOOST_AUTO_TEST_CASE(test_output_int)
{
    typedef ostreamable<_a, int> test_concept;
    std::ostringstream ss;
    any<test_concept, _a&> x(ss);
    x << 17;
    BOOST_CHECK_EQUAL(ss.str(), "17");
}

BOOST_AUTO_TEST_CASE(test_output_int_wide)
{
    typedef ostreamable<_a, int> test_concept;
    std::wostringstream ss;
    any<test_concept, _a&> x(ss);
    x << 17;
    BOOST_CHECK(ss.str() == L"17");
}

BOOST_AUTO_TEST_CASE(test_output_int_any)
{
    typedef boost::mpl::vector<ostreamable<>, copy_constructible<> > test_concept;
    std::ostringstream ss;
    any<test_concept> x(10);
    ss << x;
    BOOST_CHECK_EQUAL(ss.str(), "10");
}

BOOST_AUTO_TEST_CASE(test_output_int_any_wide)
{
    typedef boost::mpl::vector<ostreamable<std::wostream>, copy_constructible<> > test_concept;
    std::wostringstream ss;
    any<test_concept> x(10);
    ss << x;
    BOOST_CHECK(ss.str() == L"10");
}

BOOST_AUTO_TEST_CASE(test_output_both_any)
{
    typedef boost::mpl::vector<ostreamable<_a>, copy_constructible<> > test_concept;
    std::ostringstream ss;
    int val = 19;
    tuple<test_concept, _a&, _self> t(ss, val);
    get<0>(t) << get<1>(t);
    BOOST_CHECK_EQUAL(ss.str(), "19");
}

BOOST_AUTO_TEST_CASE(test_output_both_any_wide)
{
    typedef boost::mpl::vector<ostreamable<_a>, copy_constructible<> > test_concept;
    std::wostringstream ss;
    int val = 19;
    tuple<test_concept, _a&, _self> t(ss, val);
    get<0>(t) << get<1>(t);
    BOOST_CHECK(ss.str() == L"19");
}

BOOST_AUTO_TEST_CASE(test_output_overload_all)
{
    typedef boost::mpl::vector<
        ostreamable<_a>,
        ostreamable<_a, int>,
        ostreamable<_b>,
        ostreamable<_b, int>,
        ostreamable<>,
        ostreamable<std::wostream>,
        copy_constructible<>
    > test_concept;
    {
        std::ostringstream ss;
        std::wostringstream wss;
        int val = 2;
        tuple<test_concept, _a&, _b&, _self> t(ss, wss, val);
        get<0>(t) << get<2>(t);
        get<1>(t) << get<2>(t);
        BOOST_CHECK_EQUAL(ss.str(), "2");
        BOOST_CHECK(wss.str() == L"2");
    }
    {
        std::ostringstream ss;
        std::wostringstream wss;
        int val = 2;
        tuple<test_concept, _a&, _b&, _self> t(ss, wss, val);
        get<0>(t) << 3;
        get<1>(t) << 3;
        BOOST_CHECK_EQUAL(ss.str(), "3");
        BOOST_CHECK(wss.str() == L"3");
    }
    {
        std::ostringstream ss;
        std::wostringstream wss;
        int val = 5;
        tuple<test_concept, _a&, _b&, _self> t(ss, wss, val);
        ss << get<2>(t);
        wss << get<2>(t);
        BOOST_CHECK_EQUAL(ss.str(), "5");
        BOOST_CHECK(wss.str() == L"5");
    }
    {
        std::ostringstream ss;
        std::wostringstream wss;
        int val = 5;
        tuple<test_concept, _a&, _b&, _self> t(ss, wss, val);
        // we can't do anything with these, but it should
        // still compile.
        any<test_concept, const _a&> os(get<0>(t));
        any<test_concept, const _b&> wos(get<1>(t));
    }
}


BOOST_AUTO_TEST_CASE(test_input_int)
{
    typedef istreamable<_a, int> test_concept;
    std::istringstream ss("17");
    int i;
    any<test_concept, _a&> x(ss);
    x >> i;
    BOOST_CHECK_EQUAL(i, 17);
}

BOOST_AUTO_TEST_CASE(test_input_int_wide)
{
    typedef istreamable<_a, int> test_concept;
    std::wistringstream ss(L"17");
    int i;
    any<test_concept, _a&> x(ss);
    x >> i;
    BOOST_CHECK_EQUAL(i, 17);
}

BOOST_AUTO_TEST_CASE(test_input_int_any)
{
    typedef istreamable<> test_concept;
    std::istringstream ss("10");
    int i;
    any<test_concept, _self&> x(i);
    ss >> x;
    BOOST_CHECK_EQUAL(i, 10);
}

BOOST_AUTO_TEST_CASE(test_input_int_any_wide)
{
    typedef istreamable<std::wistream> test_concept;
    std::wistringstream ss(L"10");
    int i;
    any<test_concept, _self&> x(i);
    ss >> x;
    BOOST_CHECK_EQUAL(i, 10);
}

BOOST_AUTO_TEST_CASE(test_input_both_any)
{
    typedef istreamable<_a> test_concept;
    std::istringstream ss("19");
    int i;
    tuple<test_concept, _a&, _self&> t(ss, i);
    get<0>(t) >> get<1>(t);
    BOOST_CHECK_EQUAL(i, 19);
}

BOOST_AUTO_TEST_CASE(test_input_both_any_wide)
{
    typedef istreamable<_a> test_concept;
    std::wistringstream ss(L"19");
    int i;
    tuple<test_concept, _a&, _self&> t(ss, i);
    get<0>(t) >> get<1>(t);
    BOOST_CHECK_EQUAL(i, 19);
}

BOOST_AUTO_TEST_CASE(test_input_overload_all)
{
    typedef boost::mpl::vector<
        istreamable<_a>,
        istreamable<_a, int>,
        istreamable<_b>,
        istreamable<_b, int>,
        istreamable<>,
        istreamable<std::wistream>
    > test_concept;
    {
        std::istringstream ss("2");
        std::wistringstream wss(L"3");
        int i = 0;
        tuple<test_concept, _a&, _b&, _self&> t(ss, wss, i);
        get<0>(t) >> get<2>(t);
        BOOST_CHECK_EQUAL(i, 2);
        get<1>(t) >> get<2>(t);
        BOOST_CHECK_EQUAL(i, 3);
    }
    {
        std::istringstream ss("5");
        std::wistringstream wss(L"7");
        int i = 0;
        tuple<test_concept, _a&, _b&, _self&> t(ss, wss, i);
        get<0>(t) >> i;
        BOOST_CHECK_EQUAL(i, 5);
        get<1>(t) >> i;
        BOOST_CHECK_EQUAL(i, 7);
    }
    {
        std::istringstream ss("11");
        std::wistringstream wss(L"13");
        int i = 0;
        tuple<test_concept, _a&, _b&, _self&> t(ss, wss, i);
        ss >> get<2>(t);
        BOOST_CHECK_EQUAL(i, 11);
        wss >> get<2>(t);
        BOOST_CHECK_EQUAL(i, 13);
    }
    {
        std::istringstream ss;
        std::wistringstream wss;
        int val = 5;
        tuple<test_concept, _a&, _b&, _self&> t(ss, wss, val);
        // we can't do anything with these, but it should
        // still compile.
        any<test_concept, const _a&> is(get<0>(t));
        any<test_concept, const _b&> wis(get<1>(t));
    }
}
