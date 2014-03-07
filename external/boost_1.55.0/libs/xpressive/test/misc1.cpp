///////////////////////////////////////////////////////////////////////////////
// misc1.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/traits/cpp_regex_traits.hpp>
#include <boost/test/unit_test.hpp>

using namespace boost::unit_test;
using namespace boost::xpressive;

void test1()
{
    // make sure the following compiles:
    sregex a = _;
    sregex b = _;
    sregex c = a >> b;
    c = 'a' >> b;
    c = a >> 'b';
    c = a | b;
    c = 'a' | b;
    c = a | 'b';
    c = !a;
    c = *a;
    c = +a;
}

///////////////////////////////////////////////////////////////////////////////
// test for basic_regex in a keep
//
void test2()
{
    std::locale loc;
    std::string str("Its a mad Mad mAd maD world");
    sregex word = +_w;
    sregex sentence = imbue(loc)(*(keep(word) >> +_s) >> word);
    smatch what;

    BOOST_REQUIRE(regex_match(str, what, sentence));
    BOOST_REQUIRE(7 == what.nested_results().size());
    smatch::nested_results_type::const_iterator pword = what.nested_results().begin();
    BOOST_CHECK((*pword++)[0] == "Its");
    BOOST_CHECK((*pword++)[0] == "a");
    BOOST_CHECK((*pword++)[0] == "mad");
    BOOST_CHECK((*pword++)[0] == "Mad");
    BOOST_CHECK((*pword++)[0] == "mAd");
    BOOST_CHECK((*pword++)[0] == "maD");
    BOOST_CHECK((*pword++)[0] == "world");
    BOOST_CHECK(pword == what.nested_results().end());
}

///////////////////////////////////////////////////////////////////////////////
// test for a simple non-recursive grammar
//
void test3()
{
    // test for a simple regex grammar
    std::string buffer =
        "FROGGIE\r\n"
        "Volume = 1\r\n"
        "Other1= 2\r\n"
        "Channel=3\r\n"
        "Other =4\r\n"
        "\r\n"
        "FROGGIE\r\n"
        "Volume = 5\r\n"
        "Other1= 6\r\n"
        "Channel=7\r\n"
        "Other =8\r\n"
        "\r\n"
        "FROGGIE\r\n"
        "Volume = 9\r\n"
        "Other1= 0\r\n"
        "Channel=10\r\n"
        "\r\n";

    mark_tag name(1), value(2);

    sregex name_value_pair_ =
        (name= +alnum) >> *_s >> "=" >> *_s >>
        (value= +_d) >> *_s >> _ln;

    sregex message_ =
        *_s >> "FROGGIE" >> _ln >> +name_value_pair_ >> _ln;

    sregex re_ = +message_;

    smatch::nested_results_type::const_iterator msg, nvp;
    smatch tmpwhat;

    BOOST_REQUIRE(regex_search(buffer, tmpwhat, re_));
    // for giggles, make a deep-copy of the tree of results
    smatch what = tmpwhat;
    BOOST_REQUIRE(3 == what.nested_results().size());

    msg = what.nested_results().begin();
    BOOST_REQUIRE(4 == msg->nested_results().size());

    nvp = msg->nested_results().begin();
    BOOST_REQUIRE(3 == nvp->size());
    BOOST_CHECK("Volume" == (*nvp)[name]);
    BOOST_CHECK("1" == (*nvp)[value]);
    ++nvp;
    BOOST_REQUIRE(3 == nvp->size());
    BOOST_CHECK("Other1" == (*nvp)[name]);
    BOOST_CHECK("2" == (*nvp)[value]);
    ++nvp;
    BOOST_REQUIRE(3 == nvp->size());
    BOOST_CHECK("Channel" == (*nvp)[name]);
    BOOST_CHECK("3" == (*nvp)[value]);
    ++nvp;
    BOOST_REQUIRE(3 == nvp->size());
    BOOST_CHECK("Other" == (*nvp)[name]);
    BOOST_CHECK("4" == (*nvp)[value]);

    ++msg;
    BOOST_REQUIRE(4 == msg->nested_results().size());

    nvp = msg->nested_results().begin();
    BOOST_REQUIRE(3 == nvp->size());
    BOOST_CHECK("Volume" == (*nvp)[name]);
    BOOST_CHECK("5" == (*nvp)[value]);
    ++nvp;
    BOOST_REQUIRE(3 == nvp->size());
    BOOST_CHECK("Other1" == (*nvp)[name]);
    BOOST_CHECK("6" == (*nvp)[value]);
    ++nvp;
    BOOST_REQUIRE(3 == nvp->size());
    BOOST_CHECK("Channel" == (*nvp)[name]);
    BOOST_CHECK("7" == (*nvp)[value]);
    ++nvp;
    BOOST_REQUIRE(3 == nvp->size());
    BOOST_CHECK("Other" == (*nvp)[name]);
    BOOST_CHECK("8" == (*nvp)[value]);

    ++msg;
    BOOST_REQUIRE(3 == msg->nested_results().size());

    nvp = msg->nested_results().begin();
    BOOST_REQUIRE(3 == nvp->size());
    BOOST_CHECK("Volume" == (*nvp)[name]);
    BOOST_CHECK("9" == (*nvp)[value]);
    ++nvp;
    BOOST_REQUIRE(3 == nvp->size());
    BOOST_CHECK("Other1" == (*nvp)[name]);
    BOOST_CHECK("0" == (*nvp)[value]);
    ++nvp;
    BOOST_REQUIRE(3 == nvp->size());
    BOOST_CHECK("Channel" == (*nvp)[name]);
    BOOST_CHECK("10" == (*nvp)[value]);
}

///////////////////////////////////////////////////////////////////////////////
// test for a self-recursive regex
//
void test4()
{
    sregex parentheses;
    parentheses                          // A balanced set of parentheses ...
        = '('                            // is an opening parenthesis ...
            >>                           // followed by ...
             *(                          // zero or more ...
                keep( +~(set='(',')') )  // of a bunch of things that are not parentheses ...
              |                          // or ...
                by_ref(parentheses)      // a balanced set of parentheses
              )                          //   (ooh, recursion!) ...
            >>                           // followed by ...
          ')'                            // a closing parenthesis
        ;

    smatch what;
    smatch::nested_results_type::const_iterator pwhat, pwhat2;
    std::string str( "blah blah( a(b)c (c(e)f (g)h )i (j)6 )blah" );

    BOOST_REQUIRE(regex_search(str, what, parentheses));
    BOOST_REQUIRE(1 == what.size());
    BOOST_CHECK("( a(b)c (c(e)f (g)h )i (j)6 )" == what[0]);

    BOOST_REQUIRE(3 == what.nested_results().size());
    pwhat = what.nested_results().begin();
    BOOST_REQUIRE(1 == pwhat->size());
    BOOST_CHECK("(b)" == (*pwhat)[0]);

    ++pwhat;
    BOOST_REQUIRE(1 == pwhat->size());
    BOOST_CHECK("(c(e)f (g)h )" == (*pwhat)[0]);

    BOOST_REQUIRE(2 == pwhat->nested_results().size());
    pwhat2 = pwhat->nested_results().begin();
    BOOST_REQUIRE(1 == pwhat2->size());
    BOOST_CHECK("(e)" == (*pwhat2)[0]);

    ++pwhat2;
    BOOST_REQUIRE(1 == pwhat2->size());
    BOOST_CHECK("(g)" == (*pwhat2)[0]);

    ++pwhat;
    BOOST_REQUIRE(1 == pwhat->size());
    BOOST_CHECK("(j)" == (*pwhat)[0]);
}

///////////////////////////////////////////////////////////////////////////////
// test for a sub-match scoping
//
void test5()
{
    sregex inner = sregex::compile( "(.)\\1" );
    sregex outer = (s1= _) >> inner >> s1;
    std::string abba("ABBA");

    BOOST_CHECK(regex_match(abba, outer));
}

///////////////////////////////////////////////////////////////////////////////
// Ye olde calculator. Test recursive grammar.
//
void test6()
{
    sregex group, factor, term, expression;

    group       = '(' >> by_ref(expression) >> ')';
    factor      = +_d | group;
    term        = factor >> *(('*' >> factor) | ('/' >> factor));
    expression  = term >> *(('+' >> term) | ('-' >> term));

    smatch what;
    std::string str("foo 9*(10+3) bar");

    BOOST_REQUIRE(regex_search(str, what, expression));
    BOOST_CHECK("9*(10+3)" == what[0]);
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("miscelaneous tests and examples from the docs");

    test->add(BOOST_TEST_CASE(&test1));
    test->add(BOOST_TEST_CASE(&test2));
    test->add(BOOST_TEST_CASE(&test3));
    test->add(BOOST_TEST_CASE(&test4));
    test->add(BOOST_TEST_CASE(&test5));
    test->add(BOOST_TEST_CASE(&test6));

    return test;
}
