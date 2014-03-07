//  Boost string_algo library substr_test.cpp file  ------------------//

//  Copyright Pavol Droba 2002-2003. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

// Include unit test framework
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>
#include <iostream>
#include <iterator>
#include <sstream>
#include <boost/test/test_tools.hpp>

using namespace std;
using namespace boost;

void find_test()
{
    string str1("123abcxXxabcXxXabc321");
    string str2("abc");
    string str3("");
    const char* pch1="123abcxxxabcXXXabc321";
    vector<int> vec1( str1.begin(), str1.end() );

    // find results ------------------------------------------------------------//
    iterator_range<string::iterator> nc_result;
    iterator_range<string::const_iterator> cv_result;
    
    iterator_range<vector<int>::iterator> nc_vresult;
    iterator_range<vector<int>::const_iterator> cv_vresult;

    iterator_range<const char*> ch_result;

    // basic tests ------------------------------------------------------------//


    // find_first
    BOOST_TEST_CHECKPOINT( "find_first" );

    nc_result=find_first( str1, string("abc") );
    BOOST_CHECK( 
        ( (nc_result.begin()-str1.begin()) == 3) &&
        ( (nc_result.end()-str1.begin()) == 6) );

    cv_result=find_first( const_cast<const string&>(str1), str2 );
    BOOST_CHECK( 
        ( (cv_result.begin()-str1.begin()) == 3) &&
        ( (cv_result.end()-str1.begin()) == 6) );

    cv_result=ifind_first( const_cast<const string&>(str1), "xXX" );
    BOOST_CHECK( 
        ( (cv_result.begin()-str1.begin()) == 6) &&
        ( (cv_result.end()-str1.begin()) == 9) );

    ch_result=find_first( pch1, "abc" );
    BOOST_CHECK(( (ch_result.begin() - pch1 ) == 3) && ( (ch_result.end() - pch1 ) == 6 ) );

    // find_last
    BOOST_TEST_CHECKPOINT( "find_last" );
    
    nc_result=find_last( str1, string("abc") );
    BOOST_CHECK( 
        ( (nc_result.begin()-str1.begin()) == 15) &&
        ( (nc_result.end()-str1.begin()) == 18) );

    cv_result=find_last( const_cast<const string&>(str1), str2 );
    BOOST_CHECK( 
        ( (cv_result.begin()-str1.begin()) == 15) &&
        ( (cv_result.end()-str1.begin()) == 18) );

    cv_result=ifind_last( const_cast<const string&>(str1), "XXx" );
    BOOST_CHECK( 
        ( (cv_result.begin()-str1.begin()) == 12) &&
        ( (cv_result.end()-str1.begin()) == 15) );

    ch_result=find_last( pch1, "abc" );
    BOOST_CHECK(( (ch_result.begin() - pch1 ) == 15) && ( (ch_result.end() - pch1 ) == 18 ) );

    // find_nth
    BOOST_TEST_CHECKPOINT( "find_nth" );

    nc_result=find_nth( str1, string("abc"), 1 );
    BOOST_CHECK( 
        ( (nc_result.begin()-str1.begin()) == 9) &&
        ( (nc_result.end()-str1.begin()) == 12) );

    nc_result=find_nth( str1, string("abc"), -1 );
    BOOST_CHECK( 
        ( (nc_result.begin()-str1.begin()) == 15) &&
        ( (nc_result.end()-str1.begin()) == 18) );


    cv_result=find_nth( const_cast<const string&>(str1), str2, 1 );
    BOOST_CHECK( 
        ( (cv_result.begin()-str1.begin()) == 9) &&
        ( (cv_result.end()-str1.begin()) == 12) );

    cv_result=find_nth( const_cast<const string&>(str1), str2, -1 );
    BOOST_CHECK( 
        ( (cv_result.begin()-str1.begin()) == 15) &&
        ( (cv_result.end()-str1.begin()) == 18) );
        
    cv_result=ifind_nth( const_cast<const string&>(str1), "xxx", 1 );
    BOOST_CHECK( 
        ( (cv_result.begin()-str1.begin()) == 12) &&
        ( (cv_result.end()-str1.begin()) == 15) );

    cv_result=ifind_nth( const_cast<const string&>(str1), "xxx", 1 );
    BOOST_CHECK( 
        ( (cv_result.begin()-str1.begin()) == 12) &&
        ( (cv_result.end()-str1.begin()) == 15) );


    ch_result=find_nth( pch1, "abc", 1 );
    BOOST_CHECK(( (ch_result.begin() - pch1 ) == 9) && ( (ch_result.end() - pch1 ) == 12 ) );

    // find_head
    BOOST_TEST_CHECKPOINT( "find_head" );

    nc_result=find_head( str1, 6 );
    BOOST_CHECK( 
        ( (nc_result.begin()-str1.begin()) == 0) &&
        ( (nc_result.end()-str1.begin()) == 6) );

    nc_result=find_head( str1, -6 );
    BOOST_CHECK( 
        ( (nc_result.begin()-str1.begin()) == 0) &&
        ( (str1.end()-nc_result.end()) == 6 ) );

    cv_result=find_head( const_cast<const string&>(str1), 6 );
    BOOST_CHECK( 
        ( (cv_result.begin()-str1.begin()) == 0) &&
        ( (cv_result.end()-str1.begin()) == 6) );

    ch_result=find_head( pch1, 6 );
    BOOST_CHECK( ( (ch_result.begin() - pch1 ) == 0 ) && ( (ch_result.end() - pch1 ) == 6 ) );

    // find_tail
    BOOST_TEST_CHECKPOINT( "find_tail" );

    nc_result=find_tail( str1, 6 );
    BOOST_CHECK( 
        ( (nc_result.begin()-str1.begin()) == 15) &&
        ( (nc_result.end()-str1.begin()) == 21) );

    nc_result=find_tail( str1, -6 );
    BOOST_CHECK( 
        ( (nc_result.begin()-str1.begin()) == 6) &&
        ( (nc_result.end()-str1.begin()) == 21) );


    cv_result=find_tail( const_cast<const string&>(str1), 6 );
    BOOST_CHECK( 
        ( (cv_result.begin()-str1.begin()) == 15) &&
        ( (cv_result.end()-str1.begin()) == 21) );

    ch_result=find_tail( pch1, 6 );
    BOOST_CHECK( ( (ch_result.begin() - pch1 ) == 15 ) && ( (ch_result.end() - pch1 ) == 21 ) );

    // find_token
    BOOST_TEST_CHECKPOINT( "find_token" );

    nc_result=find_token( str1, is_any_of("abc"), token_compress_on );
    BOOST_CHECK( 
        ( (nc_result.begin()-str1.begin()) == 3) &&
        ( (nc_result.end()-str1.begin()) == 6) );

    cv_result=find_token( const_cast<const string&>(str1), is_any_of("abc"), token_compress_on );
    BOOST_CHECK( 
        ( (cv_result.begin()-str1.begin()) == 3) &&
        ( (cv_result.end()-str1.begin()) == 6) );

    string s1("abc def ghi jkl");
    find_iterator<string::iterator> fEnd;

    find_iterator<string::iterator> fxIt = make_find_iterator(s1,
            token_finder(is_alnum(), token_compress_on));
    BOOST_CHECK((fxIt != fEnd) && (*fxIt == string("abc")));
    ++fxIt;
    BOOST_CHECK((fxIt != fEnd) && (*fxIt == string("def")));
    ++fxIt;
    BOOST_CHECK((fxIt != fEnd) && (*fxIt == string("ghi")));
    ++fxIt;
    BOOST_CHECK((fxIt != fEnd) && (*fxIt == string("jkl")));
    ++fxIt;
    BOOST_CHECK(fxIt == fEnd);

    nc_result=find_token( str1, is_any_of("abc"), token_compress_off );
    BOOST_CHECK( 
        ( (nc_result.begin()-str1.begin()) == 3) &&
        ( (nc_result.end()-str1.begin()) == 4) );

    cv_result=find_token( const_cast<const string&>(str1), is_any_of("abc"), token_compress_off );
    BOOST_CHECK( 
        ( (cv_result.begin()-str1.begin()) == 3) &&
        ( (cv_result.end()-str1.begin()) == 4) );

    ch_result=find_token( pch1, is_any_of("abc"), token_compress_off );
    BOOST_CHECK( ( (ch_result.begin() - pch1 ) == 3 ) && ( (ch_result.end() - pch1 ) == 4 ) );

    // generic find
    BOOST_TEST_CHECKPOINT( "generic find" );

    nc_result=find(str1, first_finder(string("abc")));
    BOOST_CHECK( 
        ( (nc_result.begin()-str1.begin()) == 3) &&
        ( (nc_result.end()-str1.begin()) == 6) );

    cv_result=find(const_cast<const string&>(str1), first_finder(str2) );
    BOOST_CHECK( 
        ( (cv_result.begin()-str1.begin()) == 3) &&
        ( (cv_result.end()-str1.begin()) == 6) );

    // multi-type comparison test 
    BOOST_TEST_CHECKPOINT( "multi-type" );

    nc_vresult=find_first( vec1, string("abc") );
    BOOST_CHECK( 
        ( (nc_result.begin()-str1.begin()) == 3) &&
        ( (nc_result.end()-str1.begin()) == 6) );

    cv_vresult=find_first( const_cast<const vector<int>&>(vec1), str2 );
    BOOST_CHECK( 
        ( (cv_result.begin()-str1.begin()) == 3) &&
        ( (cv_result.end()-str1.begin()) == 6) );

    // overflow test
    BOOST_TEST_CHECKPOINT( "overflow" );
    
    nc_result=find_first( str2, string("abcd") );
    BOOST_CHECK( nc_result.begin()==nc_result.end() );
    cv_result=find_first( const_cast<const string&>(str2), string("abcd") );
    BOOST_CHECK( cv_result.begin()==cv_result.end() );

    cv_result=find_head( const_cast<const string&>(str2), 4 );
    BOOST_CHECK( string( cv_result.begin(), cv_result.end() )== string("abc") );
    cv_result=find_tail( const_cast<const string&>(str2), 4 );
    BOOST_CHECK( string( cv_result.begin(), cv_result.end() )== string("abc") );

    // Empty string test
    BOOST_TEST_CHECKPOINT( "empty" );
    
    nc_result=find_first( str3, string("abcd") );
    BOOST_CHECK( nc_result.begin()==nc_result.end() );
    nc_result=find_first( str1, string("") );
    BOOST_CHECK( nc_result.begin()==nc_result.end() );

    cv_result=find_first( const_cast<const string&>(str3), string("abcd") );
    BOOST_CHECK( cv_result.begin()==cv_result.end() );
    cv_result=find_first( const_cast<const string&>(str1), string("") );
    BOOST_CHECK( cv_result.begin()==cv_result.end() ); 

    // iterator_range specific tests
    ostringstream osstr;
    osstr << find_first( str1, "abc" );
    BOOST_CHECK( osstr.str()=="abc" );

}

// test main 
BOOST_AUTO_TEST_CASE( test_main )
{
    find_test();
}
