//  Boost string_algo library conv_test.cpp file  ---------------------------//

//  Copyright Pavol Droba 2002-2003. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/algorithm/string/case_conv.hpp>

// Include unit test framework
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <string>
#include <iostream>
#include <algorithm>
#include <boost/test/test_tools.hpp>

using namespace std;
using namespace boost;

void conv_test()
{
    string str1("AbCdEfG 123 xxxYYYzZzZ");
    string str2("AbCdEfG 123 xxxYYYzZzZ");
    string str3("");
    const char pch[]="AbCdEfG 123 xxxYYYzZzZ";    
    unsigned int pchlen=sizeof(pch);

    char* pch1=new char[pchlen];
    std::copy(pch, pch+pchlen, pch1);
    char* pch2=new char[pchlen];
    std::copy(pch, pch+pchlen, pch2);

    // *** iterator tests *** //

    string strout;
    to_lower_copy( back_inserter(strout), str1 );
    BOOST_CHECK( strout=="abcdefg 123 xxxyyyzzzz" );
    strout.clear();
    to_upper_copy( back_inserter(strout), str1 );
    BOOST_CHECK( strout=="ABCDEFG 123 XXXYYYZZZZ" );

    strout.clear();
    to_lower_copy( back_inserter(strout), "AbCdEfG 123 xxxYYYzZzZ" );
    BOOST_CHECK( strout=="abcdefg 123 xxxyyyzzzz" );
    strout.clear();
    to_upper_copy( back_inserter(strout), "AbCdEfG 123 xxxYYYzZzZ" );
    BOOST_CHECK( strout=="ABCDEFG 123 XXXYYYZZZZ" );

    strout.clear();
    to_lower_copy( back_inserter(strout), pch1 );
    BOOST_CHECK( strout=="abcdefg 123 xxxyyyzzzz" );
    strout.clear();
    to_upper_copy( back_inserter(strout), pch1 );
    BOOST_CHECK( strout=="ABCDEFG 123 XXXYYYZZZZ" );

    // *** value passing tests *** //

    BOOST_CHECK( to_lower_copy( str1 )=="abcdefg 123 xxxyyyzzzz" );
    BOOST_CHECK( to_upper_copy( str1 )=="ABCDEFG 123 XXXYYYZZZZ" );

    BOOST_CHECK( to_lower_copy( str3 )=="" );
    BOOST_CHECK( to_upper_copy( str3 )=="" );

    // *** inplace tests *** //

    to_lower( str1 );
    BOOST_CHECK( str1=="abcdefg 123 xxxyyyzzzz" );
    to_upper( str2 );
    BOOST_CHECK( str2=="ABCDEFG 123 XXXYYYZZZZ" );

    // c-string modification
    to_lower( pch1 );
    BOOST_CHECK( string(pch1)=="abcdefg 123 xxxyyyzzzz" );
    to_upper( pch2 );
    BOOST_CHECK( string(pch2)=="ABCDEFG 123 XXXYYYZZZZ" );

    to_lower( str3 );
    BOOST_CHECK( str3=="" );
    to_upper( str3 );
    BOOST_CHECK( str3=="" );

    delete[] pch1;
    delete[] pch2;
}

// test main 
BOOST_AUTO_TEST_CASE( test_main )
{
    conv_test();
}
