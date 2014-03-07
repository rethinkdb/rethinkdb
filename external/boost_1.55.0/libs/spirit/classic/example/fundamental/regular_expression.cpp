/*=============================================================================
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  Demonstrate regular expression parser objects
//  See the "Regular Expression Parser" chapter in the User's Guide.
//
//  This sample requires an installed version of the boost regex library
//  (http://www.boost.org) The sample was tested with boost V1.28.0
//
///////////////////////////////////////////////////////////////////////////////
#include <string>
#include <iostream>

#include <boost/version.hpp>
#include <boost/spirit/include/classic_core.hpp>

///////////////////////////////////////////////////////////////////////////////
//
//  The following header must be included, if regular expression support is
//  required for Spirit.
//
//  The BOOST_SPIRIT_NO_REGEX_LIB PP constant should be defined, if you're
//  using the Boost.Regex library from one translation unit only. Otherwise
//  you have to link with the Boost.Regex library as defined in the related
//  documentation (see. http://www.boost.org).
//
//  For Boost > V1.32.0 you'll always have to link against the Boost.Regex 
//  libraries.
//
///////////////////////////////////////////////////////////////////////////////
#if BOOST_VERSION <= 103200
#define BOOST_SPIRIT_NO_REGEX_LIB
#endif
#include <boost/spirit/include/classic_regex.hpp>

///////////////////////////////////////////////////////////////////////////////
//  used namespaces
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
// main entry point
int main (int argc, char *argv[])
{
    const char *ptest = "123 E 456";
    const char *prx = "[1-9]+[[:space:]]*E[[:space:]]*";

    cout << "Parse " << ptest << " against regular expression: " << prx
        << endl;

    // 1. direct use of rxlit<>
    rxstrlit<> regexpr(prx);
    parse_info<> result;
    string str;

    result = parse (ptest, regexpr[assign(str)]);
    if (result.hit)
    {
        cout << "Parsed regular expression successfully!" << endl;
        cout << "Matched (" << (int)result.length << ") characters: ";
        cout << "\"" << str << "\"" << endl;
    }
    else
    {
        cout << "Failed to parse regular expression!" << endl;
    }
    cout << endl;

    // 2. use of regex_p predefined parser object
    str.empty();
    result = parse (ptest, regex_p(prx)[assign(str)]);
    if (result.hit)
    {
        cout << "Parsed regular expression successfully!" << endl;
        cout << "Matched (" << (int)result.length << ") characters: ";
        cout << "\"" << str << "\"" << endl;
    }
    else
    {
        cout << "Failed to parse regular expression!" << endl;
    }
    cout << endl;

    // 3. test the regression reported by Grzegorz Marcin Koczyk (gkoczyk@echostar.pl)
    string str1;
    string str2;
    char const *ptest1 = "Token whatever \nToken";

    result = parse(ptest1, rxstrlit<>("Token")[assign(str1)] 
        >> rxstrlit<>("Token")[assign(str2)]);
    
    if (!result.hit)
        cout << "Parsed regular expression successfully!" << endl;
    else
        cout << "Failed to parse regular expression!" << endl;

    cout << endl;
    
    return 0;
}



