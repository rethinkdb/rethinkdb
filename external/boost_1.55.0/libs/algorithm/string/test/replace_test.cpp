//  Boost string_algo library substr_test.cpp file  ------------------//

//  Copyright Pavol Droba 2002-2003. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/std/list_traits.hpp>
#include <boost/algorithm/string/std/string_traits.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/formatter.hpp>
#include <boost/algorithm/string/classification.hpp>

// Include unit test framework
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>
#include <list>
#include <iostream>

// equals predicate is used for result comparison
#include <boost/algorithm/string/predicate.hpp>

#include <boost/test/test_tools.hpp>

using namespace std;
using namespace boost;

void sequence_traits_test()
{
    // basic_string traits
    BOOST_CHECK( boost::algorithm::has_native_replace<string>::value );
    BOOST_CHECK( !boost::algorithm::has_stable_iterators<string>::value );
    BOOST_CHECK( !boost::algorithm::has_const_time_insert<string>::value );    
    BOOST_CHECK( !boost::algorithm::has_const_time_erase<string>::value ); 

    // vector traits
    BOOST_CHECK( !boost::algorithm::has_native_replace< vector<char> >::value );
    BOOST_CHECK( !boost::algorithm::has_stable_iterators< vector<char> >::value );
    BOOST_CHECK( !boost::algorithm::has_const_time_insert< vector<char> >::value );    
    BOOST_CHECK( !boost::algorithm::has_const_time_erase< vector<char> >::value ); 

    // list traits
    BOOST_CHECK( !boost::algorithm::has_native_replace< list<char> >::value );
    BOOST_CHECK( boost::algorithm::has_stable_iterators< list<char> >::value );
    BOOST_CHECK( boost::algorithm::has_const_time_insert< list<char> >::value );   
    BOOST_CHECK( boost::algorithm::has_const_time_erase< list<char> >::value );    
}

// Combine tests for all variants of the algorithm
#define C_ ,
#define TEST_ALGO( Algo, Input, Params, Output ) \
{\
    BOOST_TEST_CHECKPOINT( #Algo " - Copy" );\
\
    string str1(Input);\
\
    /* Copy test */ \
    BOOST_CHECK( Algo##_copy( str1, Params )==Output );\
\
    BOOST_TEST_CHECKPOINT( #Algo " - Iterator" );\
    /* Iterator test */\
    string strout;\
    Algo##_copy( back_inserter(strout), str1, Params );\
    BOOST_CHECK( strout==Output ); \
\
    /* In-place test */\
    vector<char> vec1( str1.begin(), str1.end() );\
    list<char> list1( str1.begin(), str1.end() );\
\
    BOOST_TEST_CHECKPOINT( #Algo " - Inplace(string)" );\
    Algo( str1, Params ); \
    BOOST_CHECK( equals( str1, Output ) ); \
\
    BOOST_TEST_CHECKPOINT( #Algo " - Inplace(vector)" );\
    Algo( vec1, Params ); \
    BOOST_CHECK( equals( vec1, Output ) );\
\
    BOOST_TEST_CHECKPOINT( #Algo " - Inplace(list)" );\
    Algo( list1, Params ); \
    BOOST_CHECK( equals( list1, Output ) );\
}

void replace_first_test()
{
    // replace first
    TEST_ALGO( replace_first, "1abc3abc2", string("abc") C_ string("YYY"), string("1YYY3abc2") );
    TEST_ALGO( ireplace_first, "1AbC3abc2", "aBc" C_ "YYY", string("1YYY3abc2") );
    TEST_ALGO( replace_first, "1abc3abc2", string("abc") C_ string("Z"), string("1Z3abc2") );
    TEST_ALGO( replace_first, "1abc3abc2", string("abc") C_ string("XXXX"), string("1XXXX3abc2") );
    TEST_ALGO( replace_first, "1abc3abc2", string("") C_ string("XXXX"), string("1abc3abc2") );
    TEST_ALGO( replace_first, "1abc3abc2", "" C_ "XXXX", string("1abc3abc2") );
    TEST_ALGO( replace_first, "", string("") C_ string("XXXX"), string("") );
    TEST_ALGO( erase_first, "1abc3abc2", string("abc"), string("13abc2") );
    TEST_ALGO( ierase_first, "1aBc3abc2", "abC", "13abc2" );
    TEST_ALGO( erase_first, "1abc3abc2", "abc", "13abc2" );
    TEST_ALGO( erase_first, "1abc3abc2", string(""), string("1abc3abc2") );
    TEST_ALGO( erase_first, "", string("abc"), string("") );
}

void replace_last_test()
{
    // replace last
    TEST_ALGO( replace_last, "1abc3abc2", string("abc") C_ string("YYY"), string("1abc3YYY2") );
    TEST_ALGO( ireplace_last, "1abc3AbC2", "aBc" C_ "YYY", string("1abc3YYY2") );
    TEST_ALGO( replace_last, "1abc3abc2", string("abc") C_ string("Z"), string("1abc3Z2") );
    TEST_ALGO( replace_last, "1abc3abc2", string("abc") C_ string("XXXX"), string("1abc3XXXX2") );
    TEST_ALGO( replace_last, "1abc3abc2", "abc" C_ "XXXX", string("1abc3XXXX2") );
    TEST_ALGO( replace_last, "", string("") C_ string("XXXX"), string("") );
    TEST_ALGO( erase_last, "1abc3abc2", string("abc"), string("1abc32") );
    TEST_ALGO( ierase_last, "1aBc3aBc2", "ABC", string("1aBc32") );
    TEST_ALGO( erase_last, "1abc3abc2", "abc", string("1abc32") );
    TEST_ALGO( erase_last, "1abc3abc2", string(""), string("1abc3abc2") );
    TEST_ALGO( erase_last, "", string("abc"), string("") );
}

void replace_all_test()
{
    // replace all
    TEST_ALGO( replace_all, "1abc3abc2", string("abc") C_ string("YYY"), string("1YYY3YYY2") );
    TEST_ALGO( replace_all, string("1abc3abc2"), "/" C_ "\\", string("1abc3abc2") );
    TEST_ALGO( ireplace_all, "1aBc3AbC2", "abC" C_ "YYY", string("1YYY3YYY2") );
    TEST_ALGO( replace_all, "1abc3abc2", string("abc") C_ string("Z"), string("1Z3Z2") );
    TEST_ALGO( replace_all, "1abc3abc2", string("abc") C_ string("XXXX"), string("1XXXX3XXXX2") );
    TEST_ALGO( replace_all, "1abc3abc2", "abc" C_ "XXXX", string("1XXXX3XXXX2") );
    TEST_ALGO( replace_all, "", string("") C_ string("XXXX"), string("") );
    TEST_ALGO( erase_all, "1abc3abc2", string("abc"), string("132") );
    TEST_ALGO( ierase_all, "1aBc3aBc2", "aBC", string("132") );
    TEST_ALGO( erase_all, "1abc3abc2", "abc", string("132") );
    TEST_ALGO( erase_all, "1abc3abc2", string(""), string("1abc3abc2") );
    TEST_ALGO( erase_all, "", string("abc"), string("") );
}

void replace_nth_test()
{
    // replace nth
    TEST_ALGO( replace_nth, "1abc3abc2", string("abc") C_ 0 C_ string("YYY"), string("1YYY3abc2") );
    TEST_ALGO( replace_nth, "1abc3abc2", string("abc") C_ -1 C_ string("YYY"), string("1abc3YYY2") );
    TEST_ALGO( ireplace_nth, "1AbC3abc2", "aBc" C_ 0 C_ "YYY", string("1YYY3abc2") );
    TEST_ALGO( ireplace_nth, "1AbC3abc2", "aBc" C_ -1 C_ "YYY", string("1AbC3YYY2") );
    TEST_ALGO( replace_nth, "1abc3abc2", string("abc") C_ 0 C_ string("Z"), string("1Z3abc2") );
    TEST_ALGO( replace_nth, "1abc3abc2", string("abc") C_ 0 C_ string("XXXX"), string("1XXXX3abc2") );
    TEST_ALGO( replace_nth, "1abc3abc2", "abc" C_ 0 C_ "XXXX", string("1XXXX3abc2") );
    TEST_ALGO( replace_nth, "1abc3abc2", "abc" C_ 3 C_ "XXXX", string("1abc3abc2") );
    TEST_ALGO( replace_nth, "1abc3abc2", "abc" C_ -3 C_ "XXXX", string("1abc3abc2") );
    TEST_ALGO( replace_nth, "1abc3abc2", string("") C_ 0 C_ string("XXXX"), string("1abc3abc2") );
    TEST_ALGO( replace_nth, "", string("") C_ 0 C_ string("XXXX"), string("") );
    TEST_ALGO( replace_nth, "", string("") C_ -1 C_ string("XXXX"), string("") );
    TEST_ALGO( erase_nth, "1abc3abc2", string("abc") C_ 0, string("13abc2") );
    TEST_ALGO( erase_nth, "1abc3abc2", string("abc") C_ -1, string("1abc32") );
    TEST_ALGO( erase_nth, "1abc3abc2", string("abc") C_ -3, string("1abc3abc2") );
    TEST_ALGO( ierase_nth, "1aBc3aBc2", "ABC" C_ 0, string("13aBc2") );
    TEST_ALGO( ierase_nth, "1aBc3aBc2", "ABC" C_ -1, string("1aBc32") );
    TEST_ALGO( ierase_nth, "1aBc3aBc2", "ABC" C_ -3, string("1aBc3aBc2") );
    TEST_ALGO( erase_nth, "1abc3abc2", "abc" C_ 0, string("13abc2") );
    TEST_ALGO( erase_nth, "1abc3abc2", string("") C_ 0, string("1abc3abc2") );
    TEST_ALGO( erase_nth, "", string("abc") C_ 0, string("") );
    TEST_ALGO( erase_nth, "", string("abc") C_ -1, string("") );
    TEST_ALGO( replace_nth, "1abc3abc2", string("abc") C_ 1 C_ string("YYY"), string("1abc3YYY2") );
    TEST_ALGO( replace_nth, "1abc3abc2", string("abc") C_ 2 C_ string("YYY"), string("1abc3abc2") );
}

void replace_head_test()
{
    // replace head
    TEST_ALGO( replace_head, "abc3abc2", 3 C_ string("YYY"), string("YYY3abc2") );
    TEST_ALGO( replace_head, "abc3abc2", -3 C_ string("YYY"), string("YYYbc2") );
    TEST_ALGO( replace_head, "abc3abc2", 3 C_ "YYY", string("YYY3abc2") );
    TEST_ALGO( replace_head, "abc", 3 C_ string("Z"), string("Z") );
    TEST_ALGO( replace_head, "abc", 6 C_ string("XXXX"), string("XXXX") );
    TEST_ALGO( replace_head, "abc", -6 C_ string("XXXX"), string("abc") );
    TEST_ALGO( replace_head, "abc3abc2", 0 C_ string("XXXX"), string("abc3abc2") );
    TEST_ALGO( replace_head, "", 4 C_ string("XXXX"), string("") );
    TEST_ALGO( replace_head, "", -4 C_ string("XXXX"), string("") );
    TEST_ALGO( erase_head, "abc3abc2", 3, string("3abc2") );
    TEST_ALGO( erase_head, "abc3abc2", -3, string("bc2") );
    TEST_ALGO( erase_head, "abc3abc2", 0, string("abc3abc2") );
    TEST_ALGO( erase_head, "", 4, string("") );
    TEST_ALGO( erase_head, "", -4, string("") );
}

void replace_tail_test()
{
    // replace tail
    TEST_ALGO( replace_tail, "abc3abc", 3 C_ string("YYY"), string("abc3YYY") );
    TEST_ALGO( replace_tail, "abc3abc", -3 C_ "YYY", string("abcYYY") );
    TEST_ALGO( replace_tail, "abc", 3 C_ string("Z"), string("Z") );
    TEST_ALGO( replace_tail, "abc", 6 C_ string("XXXX"), string("XXXX") );
    TEST_ALGO( replace_tail, "abc", -6 C_ string("XXXX"), string("abc") );
    TEST_ALGO( replace_tail, "abc3abc", 0 C_ string("XXXX"), string("abc3abc") );
    TEST_ALGO( replace_tail, "", 4 C_ string("XXXX"), string("") );
    TEST_ALGO( replace_tail, "", -4 C_ string("XXXX"), string("") );
    TEST_ALGO( erase_tail, "abc3abc", 3, string("abc3") );
    TEST_ALGO( erase_tail, "abc3abc", -3, string("abc") );
    TEST_ALGO( erase_tail, "abc3abc", 0, string("abc3abc") );
    TEST_ALGO( erase_tail, "", 4, string("") );
    TEST_ALGO( erase_tail, "", -4, string("") );
}

void replace_range_test()
{
    // replace_range
    {
        BOOST_TEST_CHECKPOINT( "replace_range" );

        string str1("1abc3abc2");
        BOOST_CHECK( 
            replace_range_copy( 
                str1, 
                make_iterator_range(str1.begin()+1, str1.begin()+4),
                string("XXX") )==string("1XXX3abc2") );

        string strout;
        replace_range_copy( 
                back_inserter( strout ),
                str1, 
                make_iterator_range(str1.begin()+1, str1.begin()+4),
                string("XXX") );
        BOOST_CHECK( strout==string("1XXX3abc2") );

        replace_range( 
                str1, 
                make_iterator_range(str1.begin()+1, str1.begin()+4),
                string("XXX") );
        BOOST_CHECK( str1==string("1XXX3abc2") );
    }
    // erase_range
    {
        BOOST_TEST_CHECKPOINT( "erase_range" );

        string str1("1abc3abc2");
        BOOST_CHECK( 
        erase_range_copy( 
                str1, 
                make_iterator_range(str1.begin()+1, str1.begin()+4))==string("13abc2") );

        string strout;
        erase_range_copy( 
                back_inserter( strout ),
                str1, 
                make_iterator_range(str1.begin()+1, str1.begin()+4));
        BOOST_CHECK( strout==string("13abc2") );

        erase_range( 
                str1, 
                make_iterator_range(str1.begin()+1, str1.begin()+4));
        BOOST_CHECK( str1==string("13abc2") );
    }
}

void collection_comp_test()
{
    // container traits compatibility tests
    {
        string strout;
        replace_first_copy( back_inserter(strout), "1abc3abc2", "abc", "YYY" );
        BOOST_CHECK( strout==string("1YYY3abc2") ); 
    }
    {
        string strout;
        replace_last_copy( back_inserter(strout), "1abc3abc2", "abc", "YYY" );
        BOOST_CHECK( strout==string("1abc3YYY2") ); 
    }   
    {
        string strout;
        replace_all_copy( back_inserter(strout), "1abc3abc2", "abc", "YYY" );
        BOOST_CHECK( strout==string("1YYY3YYY2") ); 
    }   
    {
        string strout;
        replace_nth_copy( back_inserter(strout), "1abc3abc2", "abc", 1, "YYY" );
        BOOST_CHECK( strout==string("1abc3YYY2") ); 
    }   
    {
        string strout;
        replace_head_copy( back_inserter(strout), "abc3abc2", 3 , "YYY" );
        BOOST_CHECK( strout==string("YYY3abc2") ); 
    }   
    {
        string strout;
        replace_tail_copy( back_inserter(strout), "abc3abc", 3 , "YYY" );
        BOOST_CHECK( strout==string("abc3YYY") ); 
    }   
}

void dissect_format_test()
{
    BOOST_CHECK(
        find_format_all_copy(
            string("aBc123Abc"), 
            first_finder("abc", is_iequal()), 
            dissect_formatter(token_finder(is_upper())))=="B123A");


    BOOST_CHECK(
        find_format_all_copy(
            string("abc   123   abc"),
            token_finder(is_space(), token_compress_on),
            dissect_formatter(head_finder(1)))=="abc 123 abc");

}

BOOST_AUTO_TEST_CASE( test_main )
{
    sequence_traits_test(); 
    replace_first_test();
    replace_last_test();
    replace_all_test();
    replace_nth_test();
    replace_head_test();
    replace_tail_test();
    replace_range_test();
    collection_comp_test();
    dissect_format_test();
}
