///////////////////////////////////////////////////////////////////////////////
// test_symbols.cpp
//
//  Copyright 2008 David Jenkins.
//  Copyright 2008 Eric Niebler.
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <map>
#include <boost/version.hpp>
#include <boost/xpressive/xpressive_static.hpp>
#include <boost/xpressive/regex_actions.hpp>
#include <boost/test/unit_test.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

namespace xp = boost::xpressive;

///////////////////////////////////////////////////////////////////////////////
// test1
//  simple action which builds a *translated* string
void test1()
{
    using namespace boost::xpressive;

    local<std::string> result;
    std::string str("foo bar baz foo bar baz");
    std::map<std::string,std::string> map1;
    map1["foo"] = "1";
    map1["bar"] = "2";
    map1["baz"] = "3";

    sregex rx = skip(_s) (+(a1=map1)
        [ result += if_else(length(result) > 0, ",", "") + a1 ]
    );

    if(!regex_match(str, rx))
    {
        BOOST_ERROR("oops");
    }
    else
    {
        BOOST_CHECK_EQUAL(result.get(), "1,2,3,1,2,3");
    }
}

///////////////////////////////////////////////////////////////////////////////
// test2
//  find longest match in symbol table
void test2()
{
    using namespace boost::xpressive;

    local<std::string> result;
    std::string str("foobarbazfoobazbazfoobazbar");
    std::map<std::string,std::string> map1;
    map1["foo"] = "1";
    map1["bar"] = "2";
    map1["baz"] = "3";
    map1["foobaz"] = "4";
    map1["foobazbaz"] = "5";

    sregex rx = skip(_s) (+(a1=map1)
        [ result += if_else(length(result) > 0, ",", "") + a1 ]
    );

    if(!regex_match(str, rx))
    {
        BOOST_ERROR("oops");
    }
    else
    {
        BOOST_CHECK_EQUAL(result.get(), "1,2,3,5,4,2");
    }
}

///////////////////////////////////////////////////////////////////////////////
// test3
//  *map* string to int, push back into list, use alternate ->* syntax
void test3()
{
    using namespace boost::xpressive;

    std::list<int> result;
    std::string str("foo bar baz bop");
    std::map<std::string,int> map1;
    map1["foo"] = 1;
    map1["bar"] = 23;
    map1["baz"] = 456;
    map1["bop"] = 7890;

#if BOOST_VERSION >= 103500
    sregex rx = skip(_s) (+(a1=map1)
        [ xp::ref(result)->*push_back( a1 ) ]
    );
#else
    sregex rx = skip(_s) (+(a1=map1)
        [ push_back(xp::ref(result), a1 ) ]
    );
#endif

    if(!regex_match(str, rx))
    {
        BOOST_ERROR("oops");
    }
    else
    {
        BOOST_REQUIRE_EQUAL(result.size(), 4u);
        BOOST_CHECK_EQUAL(*result.begin(), 1);
        BOOST_CHECK_EQUAL(*++result.begin(), 23);
        BOOST_CHECK_EQUAL(*++++result.begin(), 456);
        BOOST_CHECK_EQUAL(*++++++result.begin(), 7890);
    }
}

///////////////////////////////////////////////////////////////////////////////
// test4
//  use two input maps to build an output map, with a late-bound action argument.
void test4()
{
    using namespace boost::xpressive;
    placeholder< std::map<std::string, int> > const _map = {};

    std::string str("aaa=>1 bbb=>2 ccc=>3");
    std::map<std::string,std::string> map1;
    map1["aaa"] = "foo";
    map1["bbb"] = "bar";
    map1["ccc"] = "baz";
    std::map<std::string,int> map2;
    map2["1"] = 1;
    map2["2"] = 23;
    map2["3"] = 456;

    sregex pair = ( (a1=map1) >> "=>" >> (a2= map2) )[ _map[a1] = a2 ];
    sregex rx = pair >> *(+_s >> pair);

    smatch what;
    std::map<std::string, int> result;
    what.let(_map = result); // bind the argument!

    if(!regex_match(str, what, rx))
    {
        BOOST_ERROR("oops");
    }
    else
    {
        BOOST_REQUIRE_EQUAL(result.size(), 3u);
        BOOST_CHECK_EQUAL(result["foo"], 1);
        BOOST_CHECK_EQUAL(result["bar"], 23);
        BOOST_CHECK_EQUAL(result["baz"], 456);
    }
}

///////////////////////////////////////////////////////////////////////////////
// test5
//  test nine maps and attributes
void test5()
{
    using namespace boost::xpressive;

    local<int> result(0);
    std::string str("abcdefghi");
    std::map<std::string,int> map1;
    std::map<std::string,int> map2;
    std::map<std::string,int> map3;
    std::map<std::string,int> map4;
    std::map<std::string,int> map5;
    std::map<std::string,int> map6;
    std::map<std::string,int> map7;
    std::map<std::string,int> map8;
    std::map<std::string,int> map9;
    map1["a"] = 1;
    map2["b"] = 2;
    map3["c"] = 3;
    map4["d"] = 4;
    map5["e"] = 5;
    map6["f"] = 6;
    map7["g"] = 7;
    map8["h"] = 8;
    map9["i"] = 9;

    sregex rx =
           (a1=map1)[ result += a1 ]
        >> (a2=map2)[ result += a2 ]
        >> (a3=map3)[ result += a3 ]
        >> (a4=map4)[ result += a4 ]
        >> (a5=map5)[ result += a5 ]
        >> (a6=map6)[ result += a6 ]
        >> (a7=map7)[ result += a7 ]
        >> (a8=map8)[ result += a8 ]
        >> (a9=map9)[ result += a9 ];

    if(!regex_match(str, rx))
    {
        BOOST_ERROR("oops");
    }
    else
    {
        BOOST_CHECK_EQUAL(result.get(), 45);
    }
}

///////////////////////////////////////////////////////////////////////////////
// test6
//  test case-sensitivity
void test6()
{
    using namespace boost::xpressive;

    local<std::string> result;
    std::map<std::string,std::string> map1;
    map1["a"] = "1";
    map1["A"] = "2";
    map1["b"] = "3";
    map1["B"] = "4";
    std::string str("a A b B a A b B");
    sregex rx = skip(_s)(
        icase(a1= map1) [ result = a1 ]
        >> repeat<3>( (icase(a1= map1) [ result += ',' + a1 ]) )
        >> repeat<4>( ((a1= map1) [ result += ',' + a1 ]) )
        );
    if(!regex_match(str, rx))
    {
        BOOST_ERROR("oops");
    }
    else
    {
        BOOST_CHECK_EQUAL(result.get(), "1,1,3,3,1,2,3,4");
    }
}

///////////////////////////////////////////////////////////////////////////////
// test7
//  test multiple mutually-exclusive maps and default attribute value
void test7()
{
    using namespace boost::xpressive;

    local<std::string> result;
    std::map<std::string,std::string> map1;
    map1["a"] = "1";
    map1["b"] = "2";
    std::map<std::string,std::string> map2;
    map2["c"] = "3";
    map2["d"] = "4";
    std::string str("abcde");
    sregex rx = *((a1= map1) | (a1= map2) | 'e') [ result += (a1 | "9") ];
    if(!regex_match(str, rx))
    {
        BOOST_ERROR("oops");
    }
    else
    {
        BOOST_CHECK_EQUAL(result.get(), "12349");
    }
}

#ifndef BOOST_XPRESSIVE_NO_WREGEX
struct City
{
    std::wstring name;
    char const* nickname;
    int population;
};

BOOST_TYPEOF_REGISTER_TYPE(City)

///////////////////////////////////////////////////////////////////////////////
// test8
//  test wide strings with structure result
void test8()
{
    using namespace boost::xpressive;

    City cities[] = {
        {L"Chicago", "The Windy City", 945000},
        {L"New York", "The Big Apple", 16626000},
        {L"\u041c\u043E\u0441\u043A\u0432\u0430", "Moscow", 9299000}
    };
    int const nbr_cities = sizeof(cities)/sizeof(*cities);

    std::map<std::wstring, City> map1;
    for(int i=0; i<nbr_cities; ++i)
    {
        map1[cities[i].name] = cities[i];
    }

    std::wstring str(L"Chicago \u041c\u043E\u0441\u043A\u0432\u0430");
    local<City> result1, result2;
    wsregex rx = (a1= map1)[ result1 = a1 ] >> +_s
        >> (a1= map1)[ result2 = a1 ];
    if(!regex_match(str, rx))
    {
        BOOST_ERROR("oops");
    }
    else
    {
        BOOST_CHECK_EQUAL(result1.get().nickname, "The Windy City");
        BOOST_CHECK_EQUAL(result2.get().nickname, "Moscow");
    }
}
#else
void test8()
{
    // This test is empty
}
#endif

///////////////////////////////////////////////////////////////////////////////
// test9
//  test "not before" using a map
void test9()
{
    using namespace boost::xpressive;

    std::string result;
    std::string str("foobar");
    std::map<std::string,int> map1;
    map1["foo"] = 1;
    sregex rx = ~before((a1=map1)[a1]) >>
        (s1=*_w)[ xp::ref(result) = s1 ];
    if(!regex_match(str, rx))
    {
        BOOST_CHECK_EQUAL(result, "");
    }
    else
    {
        BOOST_ERROR("oops");
    }
}

using namespace boost::unit_test;

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test_symbols");
    test->add(BOOST_TEST_CASE(&test1));
    test->add(BOOST_TEST_CASE(&test2));
    test->add(BOOST_TEST_CASE(&test3));
    test->add(BOOST_TEST_CASE(&test4));
    test->add(BOOST_TEST_CASE(&test5));
    test->add(BOOST_TEST_CASE(&test6));
    test->add(BOOST_TEST_CASE(&test7));
    test->add(BOOST_TEST_CASE(&test8));
    test->add(BOOST_TEST_CASE(&test9));
    return test;
}

