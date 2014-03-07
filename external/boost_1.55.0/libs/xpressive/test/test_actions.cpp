///////////////////////////////////////////////////////////////////////////////
// test_actions.cpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//#define BOOST_XPRESSIVE_BETTER_ERRORS

#include <map>
#include <list>
#include <stack>
#include <numeric>
#include <boost/version.hpp>
#include <boost/xpressive/xpressive_static.hpp>
#include <boost/xpressive/regex_actions.hpp>
#include <boost/test/unit_test.hpp>

namespace xp = boost::xpressive;

///////////////////////////////////////////////////////////////////////////////
// test1
//  simple action which builds a string
void test1()
{
    using namespace boost::xpressive;

    std::string result;
    std::string str("foo bar baz foo bar baz");
    sregex rx = (+_w)[ xp::ref(result) += _ ] >> *(' ' >> (+_w)[ xp::ref(result) += ',' + _ ]);

    if(!regex_match(str, rx))
    {
        BOOST_ERROR("oops");
    }
    else
    {
        BOOST_CHECK_EQUAL(result, "foo,bar,baz,foo,bar,baz");
    }
}

///////////////////////////////////////////////////////////////////////////////
// test2
//  test backtracking over actions
void test2()
{
    using namespace boost::xpressive;

    std::string result;
    std::string str("foo bar baz foo bar baz");
    sregex rx = (+_w)[ xp::ref(result) += _ ] >> *(' ' >> (+_w)[ xp::ref(result) += ',' + _ ]) >> repeat<4>(_);

    if(!regex_match(str, rx))
    {
        BOOST_ERROR("oops");
    }
    else
    {
        BOOST_CHECK_EQUAL(result, "foo,bar,baz,foo,bar");
    }
}

///////////////////////////////////////////////////////////////////////////////
// test3
//  cast string to int, push back into list, use alternate ->* syntax
void test3()
{
    using namespace boost::xpressive;

    std::list<int> result;
    std::string str("1 23 456 7890");
#if BOOST_VERSION >= 103500
    sregex rx = (+_d)[ xp::ref(result)->*push_back( as<int>(_) ) ]
        >> *(' ' >> (+_d)[ xp::ref(result)->*push_back( as<int>(_) ) ]);
#else
    sregex rx = (+_d)[ push_back(xp::ref(result), as<int>(_) ) ]
        >> *(' ' >> (+_d)[ push_back(xp::ref(result), as<int>(_) ) ]);
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
//  build a map of strings to integers
void test4()
{
    using namespace boost::xpressive;

    std::map<std::string, int> result;
    std::string str("aaa=>1 bbb=>23 ccc=>456");
    sregex pair = ( (s1= +_w) >> "=>" >> (s2= +_d) )[ xp::ref(result)[s1] = as<int>(s2) ];
    sregex rx = pair >> *(+_s >> pair);

    if(!regex_match(str, rx))
    {
        BOOST_ERROR("oops");
    }
    else
    {
        BOOST_REQUIRE_EQUAL(result.size(), 3u);
        BOOST_CHECK_EQUAL(result["aaa"], 1);
        BOOST_CHECK_EQUAL(result["bbb"], 23);
        BOOST_CHECK_EQUAL(result["ccc"], 456);
    }
}

///////////////////////////////////////////////////////////////////////////////
// test4_aux
//  build a map of strings to integers, with a late-bound action argument.
void test4_aux()
{
    using namespace boost::xpressive;
    placeholder< std::map<std::string, int> > const _map = {{}};

    sregex pair = ( (s1= +_w) >> "=>" >> (s2= +_d) )[ _map[s1] = as<int>(s2) ];
    sregex rx = pair >> *(+_s >> pair);

    std::string str("aaa=>1 bbb=>23 ccc=>456");
    smatch what;
    std::map<std::string, int> result;
    what.let(_map = result); // bind the argument!

    BOOST_REQUIRE(regex_match(str, what, rx));
    BOOST_REQUIRE_EQUAL(result.size(), 3u);
    BOOST_CHECK_EQUAL(result["aaa"], 1);
    BOOST_CHECK_EQUAL(result["bbb"], 23);
    BOOST_CHECK_EQUAL(result["ccc"], 456);

    // Try the same test with regex_iterator
    result.clear();
    sregex_iterator it(str.begin(), str.end(), pair, let(_map=result)), end;
    BOOST_REQUIRE_EQUAL(3, std::distance(it, end));
    BOOST_REQUIRE_EQUAL(result.size(), 3u);
    BOOST_CHECK_EQUAL(result["aaa"], 1);
    BOOST_CHECK_EQUAL(result["bbb"], 23);
    BOOST_CHECK_EQUAL(result["ccc"], 456);

    // Try the same test with regex_token_iterator
    result.clear();
    sregex_token_iterator it2(str.begin(), str.end(), pair, (s1,s2), let(_map=result)), end2;
    BOOST_REQUIRE_EQUAL(6, std::distance(it2, end2));
    BOOST_REQUIRE_EQUAL(result.size(), 3u);
    BOOST_CHECK_EQUAL(result["aaa"], 1);
    BOOST_CHECK_EQUAL(result["bbb"], 23);
    BOOST_CHECK_EQUAL(result["ccc"], 456);
}

///////////////////////////////////////////////////////////////////////////////
// test5
//  calculator that calculates. This is just silly, but hey.
void test5()
{
    using namespace boost::xpressive;

    // test for "local" variables.
    local<int> left, right;

    // test for reference<> to an existing variable
    std::stack<int> stack_;
    reference<std::stack<int> > stack(stack_);

    std::string str("4+5*(3-1)");

    sregex group, factor, term, expression;

    group       = '(' >> by_ref(expression) >> ')';
    factor      = (+_d)[ push(stack, as<int>(_)) ] | group;
    term        = factor >> *(
                                ('*' >> factor)
                                    [ right = top(stack)
                                    , pop(stack)
                                    , left = top(stack)
                                    , pop(stack)
                                    , push(stack, left * right)
                                    ]
                              | ('/' >> factor)
                                    [ right = top(stack)
                                    , pop(stack)
                                    , left = top(stack)
                                    , pop(stack)
                                    , push(stack, left / right)
                                    ]
                             );
    expression  = term >> *(
                                ('+' >> term)
                                    [ right = top(stack)
                                    , pop(stack)
                                    , left = top(stack)
                                    , pop(stack)
                                    , push(stack, left + right)
                                    ]
                              | ('-' >> term)
                                    [ right = top(stack)
                                    , pop(stack)
                                    , left = top(stack)
                                    , pop(stack)
                                    , push(stack, left - right)
                                    ]
                             );

    if(!regex_match(str, expression))
    {
        BOOST_ERROR("oops");
    }
    else
    {
        BOOST_REQUIRE_EQUAL(stack_.size(), 1u);
        BOOST_CHECK_EQUAL(stack_.top(), 14);

        BOOST_REQUIRE_EQUAL(stack.get().size(), 1u);
        BOOST_CHECK_EQUAL(stack.get().top(), 14);
    }
}

///////////////////////////////////////////////////////////////////////////////
// test6
//  Test as<>() with wide strings. Bug #4496.
void test6()
{
    using namespace boost::xpressive;

    std::wstring version(L"0.9.500");

    local<int> maj1(0), min1(0), build1(0);

    wsregex re1 = (+_d)[maj1 = as<int>(_)] >> L"." >>
                  (+_d)[min1 = as<int>(_)] >> L"." >>
                  (+_d)[build1 = as<int>(_)];

    BOOST_REQUIRE(regex_match(version, re1));

    BOOST_CHECK_EQUAL(maj1.get(), 0);
    BOOST_CHECK_EQUAL(min1.get(), 9);
    BOOST_CHECK_EQUAL(build1.get(), 500);
}

///////////////////////////////////////////////////////////////////////////////
// test7
//  Test regex_replace with an xpressive lambda
void test7()
{
    namespace xp = boost::xpressive;
    using namespace xp;

    std::map<std::string, std::string> env;
    env["XXX"] = "!!!";
    env["YYY"] = "???";

    std::string text("This is a %XXX% string %YYY% and stuff.");
    sregex var = '%' >> (s1 = +_w) >> '%';

    text = regex_replace(text, var, xp::ref(env)[s1]);

    BOOST_CHECK_EQUAL(text, "This is a !!! string ??? and stuff.");
}

using namespace boost::unit_test;

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test_actions");
    test->add(BOOST_TEST_CASE(&test1));
    test->add(BOOST_TEST_CASE(&test2));
    test->add(BOOST_TEST_CASE(&test3));
    test->add(BOOST_TEST_CASE(&test4));
    test->add(BOOST_TEST_CASE(&test4_aux));
    test->add(BOOST_TEST_CASE(&test5));
    test->add(BOOST_TEST_CASE(&test6));
    test->add(BOOST_TEST_CASE(&test7));
    return test;
}

