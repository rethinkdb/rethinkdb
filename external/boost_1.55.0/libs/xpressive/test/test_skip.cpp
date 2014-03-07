///////////////////////////////////////////////////////////////////////////////
// test_skip.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <map>
#include <iostream>
#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/regex_actions.hpp>
#include <boost/test/unit_test.hpp>

using namespace boost::unit_test;
using namespace boost::xpressive;

void test1()
{
    std::string s = "a a b b c c";

    sregex rx =
        "a a" >>
        skip(_s)
        (
             (s1= as_xpr('b')) >>
             as_xpr('b') >>
            *as_xpr('c')    // causes backtracking
        ) >>
        "c c";

    smatch what;
    BOOST_CHECK( regex_match(s, what, rx) );

    s = "123,456,789";
    sregex rx2 = skip(',')(+_d);
    BOOST_CHECK( regex_match(s, what, rx2) );

    s = "foo";
    sregex rx3 = skip(_s)(after("fo") >> 'o');
    BOOST_CHECK( regex_search(s, what, rx3) );
}

template<typename Expr>
void test_skip_aux(Expr const &expr)
{
    sregex rx = skip(_s)(expr);
}

void test_skip()
{
    int i=0;
    std::map<std::string, int> syms;
    std::locale loc;

    test_skip_aux( 'a' );
    test_skip_aux( _ );
    test_skip_aux( +_ );
    test_skip_aux( -+_ );
    test_skip_aux( !_ );
    test_skip_aux( -!_ );
    test_skip_aux( repeat<0,42>(_) );
    test_skip_aux( -repeat<0,42>(_) );
    test_skip_aux( _ >> 'a' );
    test_skip_aux( _ >> 'a' | _ );
    test_skip_aux( _ >> 'a' | _ >> 'b' );
    test_skip_aux( s1= _ >> 'a' | _ >> 'b' );
    test_skip_aux( icase(_ >> 'a' | _ >> 'b') );
    test_skip_aux( imbue(loc)(_ >> 'a' | _ >> 'b') );
    test_skip_aux( (set='a') );
    test_skip_aux( (set='a','b') );
    test_skip_aux( ~(set='a') );
    test_skip_aux( ~(set='a','b') );
    test_skip_aux( range('a','b') );
    test_skip_aux( ~range('a','b') );
    test_skip_aux( set['a' | alpha] );
    test_skip_aux( ~set['a' | alpha] );
    test_skip_aux( before(_) );
    test_skip_aux( ~before(_) );
    test_skip_aux( after(_) );
    test_skip_aux( ~after(_) );
    test_skip_aux( keep(*_) );
    test_skip_aux( (*_)[ref(i) = as<int>(_) + 1] );
    test_skip_aux( (a1= syms)[ref(i) = a1 + 1] );
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test skip()");

    test->add(BOOST_TEST_CASE(&test1));

    return test;
}
