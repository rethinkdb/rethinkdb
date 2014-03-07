//////////////////////////////////////////////////////////////////////////////
// test_dynamic_grammar.cpp
//
//  (C) Copyright Eric Niebler 2004.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
 Revision history:
   5 March 2007 : Initial version.
*/

// defining this causes regex_impl objects to be counted, allowing us to detect
// leaks portably.
#define BOOST_XPRESSIVE_DEBUG_CYCLE_TEST

#include <boost/xpressive/xpressive.hpp>
#include <boost/test/unit_test.hpp>

void test_dynamic_grammar()
{
    using namespace boost::xpressive;

    {
        sregex expr;
        {
            sregex_compiler compiler;
            regex_constants::syntax_option_type x = regex_constants::ignore_white_space;

                   compiler.compile( "(? $group  = ) \\( (? $expr ) \\) ", x);
                   compiler.compile( "(? $factor = ) \\d+ | (? $group ) ", x);
                   compiler.compile( "(? $term   = ) (? $factor ) (?: \\* (? $factor ) | / (? $factor ) )* ", x);
            expr = compiler.compile( "(? $expr   = ) (? $term )   (?: \\+ (? $term )   | - (? $term )   )* ", x);
        }

        std::string str("foo 9*(10+3) bar");
        smatch what;

        if(regex_search(str, what, expr))
        {
             BOOST_CHECK_EQUAL(what[0].str(), "9*(10+3)");
             BOOST_CHECK_EQUAL((*what.nested_results().begin())[0].str(), "9*(10+3)");
             BOOST_CHECK_EQUAL((*(*what.nested_results().begin()).nested_results().begin())[0].str(), "9");
             BOOST_CHECK_EQUAL((*++(*what.nested_results().begin()).nested_results().begin())[0].str(), "(10+3)");
        }
        else
        {
            BOOST_ERROR("regex_search test 1 failed");
        }
    }

    // Test that all regex_impl instances have been cleaned up correctly
    BOOST_CHECK_EQUAL(0, detail::regex_impl<std::string::const_iterator>::instances);
}

void test_dynamic_grammar2()
{
    using namespace boost::xpressive;

    {
        sregex expr;
        {
            sregex_compiler compiler;
            regex_constants::syntax_option_type x = regex_constants::ignore_white_space;

            compiler["group"]   = compiler.compile( "\\( (? $expr ) \\) ", x);
            compiler["factor"]  = compiler.compile( "\\d+ | (? $group ) ", x);
            compiler["term"]    = compiler.compile( "(? $factor ) (?: \\* (? $factor ) | / (? $factor ) )* ", x);
            compiler["expr"]    = compiler.compile( "(? $term )   (?: \\+ (? $term )   | - (? $term )   )* ", x);

            expr = compiler["expr"];
        }

        std::string str("foo 9*(10+3) bar");
        smatch what;

        if(regex_search(str, what, expr))
        {
             BOOST_CHECK_EQUAL(what[0].str(), "9*(10+3)");
             BOOST_CHECK_EQUAL((*what.nested_results().begin())[0].str(), "9*(10+3)");
             BOOST_CHECK_EQUAL((*(*what.nested_results().begin()).nested_results().begin())[0].str(), "9");
             BOOST_CHECK_EQUAL((*++(*what.nested_results().begin()).nested_results().begin())[0].str(), "(10+3)");
        }
        else
        {
            BOOST_ERROR("regex_search test 2 failed");
        }
    }

    // Test that all regex_impl instances have been cleaned up correctly
    BOOST_CHECK_EQUAL(0, detail::regex_impl<std::string::const_iterator>::instances);
}

using namespace boost;
using namespace unit_test;

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("testing dynamic grammars");
    test->add(BOOST_TEST_CASE(&test_dynamic_grammar));
    test->add(BOOST_TEST_CASE(&test_dynamic_grammar2));
    return test;
}
