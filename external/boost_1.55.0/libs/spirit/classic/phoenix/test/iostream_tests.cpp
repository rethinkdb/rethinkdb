/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2003 Joel de Guzman

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <boost/detail/lightweight_test.hpp>

#include <boost/config.hpp>
#ifdef BOOST_NO_STRINGSTREAM
#include <strstream>
#define SSTREAM strstream
std::string GETSTRING(std::strstream& ss)
{
    ss << ends;
    std::string rval = ss.str();
    ss.freeze(false);
    return rval;
}
#else
#include <sstream>
#define GETSTRING(ss) ss.str()
#define SSTREAM stringstream
#endif

//#define PHOENIX_LIMIT 15
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_composite.hpp>
#include <boost/spirit/include/phoenix1_operators.hpp>
#include <boost/spirit/include/phoenix1_special_ops.hpp>

using namespace phoenix;
using namespace std;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    int     i100 = 100;
    string hello = "hello";
    const char* world = " world";

///////////////////////////////////////////////////////////////////////////////
//
//  IO streams
//
///////////////////////////////////////////////////////////////////////////////
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    v.push_back(5);

    char const* msg = "cout assert\n";
    (cout << arg1)(msg);
    (cout << val(hello) << world << ", you da man!\n")();
    for_each(v.begin(), v.end(), cout << arg1 << ',');
    cout << endl;

#ifdef __BORLANDC__ // *** See special_ops.hpp why ***
    (cout << arg1 << "this is it, shukz:" << hex_ << arg2 << endl_ << endl_)(msg, i100);
#else
    (cout << arg1 << "this is it, shukz:" << hex << arg2 << endl << endl)(msg, i100);
#endif
    int in;
    int out = 12345;
    SSTREAM sstr;
    (sstr << arg1)(out);
    (sstr >> arg1)(in);
    BOOST_TEST(in == out);

///////////////////////////////////////////////////////////////////////////////
//
//  End asserts
//
///////////////////////////////////////////////////////////////////////////////

    return boost::report_errors();    
}
