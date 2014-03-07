/*=============================================================================
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//  This example shows the usage of the refactoring parser family parsers
//  See the "Refactoring Parsers" chapter in the User's Guide.

#include <iostream>
#include <string>

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_refactoring.hpp>

///////////////////////////////////////////////////////////////////////////////
// used namespaces
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
// actor, used by the refactor_action_p test
struct refactor_action_actor
{
    refactor_action_actor (std::string &str_) : str(str_) {}

    template <typename IteratorT>
    void operator() (IteratorT const &first, IteratorT const &last) const
    {
        str = std::string(first, last-first);
    }

    std::string &str;
};

///////////////////////////////////////////////////////////////////////////////
// main entry point
int main()
{
    parse_info<> result;
    char const *test_string = "Some string followed by a newline\n";

///////////////////////////////////////////////////////////////////////////////
//
//  1. Testing the refactor_unary_d parser
//
//  The following test should successfully parse the test string, because the
//
//      refactor_unary_d[
//          *anychar_p - '\n'
//      ]
//
//  is refactored into
//
//      *(anychar_p - '\n').
//
///////////////////////////////////////////////////////////////////////////////

    result = parse(test_string, refactor_unary_d[*anychar_p - '\n'] >> '\n');

    if (result.full)
    {
        cout << "Successfully refactored an unary!" << endl;
    }
    else
    {
        cout << "Failed to refactor an unary!" << endl;
    }

//  Parsing the same test string without refactoring fails, because the
//  *anychar_p eats up all the input up to the end of the input string.

    result = parse(test_string, (*anychar_p - '\n') >> '\n');

    if (result.full)
    {
        cout
            << "Successfully parsed test string (should not happen)!"
            << endl;
    }
    else
    {
        cout
            << "Correctly failed parsing the test string (without refactoring)!"
            << endl;
    }
    cout << endl;

///////////////////////////////////////////////////////////////////////////////
//
//  2. Testing the refactor_action_d parser
//
//  The following test should successfully parse the test string, because the
//
//      refactor_action_d[
//          (*(anychar_p - '$'))[refactor_action_actor(str)] >> '$'
//      ]
//
//  is refactored into
//
//      (*(anychar_p - '$') >> '$')[refactor_action_actor(str)].
//
///////////////////////////////////////////////////////////////////////////////

    std::string str;
    char const *test_string2 = "Some test string ending with a $";

    result =
        parse(test_string2,
            refactor_action_d[
                (*(anychar_p - '$'))[refactor_action_actor(str)] >> '$'
            ]
        );

    if (result.full && str == std::string(test_string2))
    {
        cout << "Successfully refactored an action!" << endl;
        cout << "Parsed: \"" << str << "\"" << endl;
    }
    else
    {
        cout << "Failed to refactor an action!" << endl;
    }

//  Parsing the same test string without refactoring fails, because the
//  the attached actor gets called only for the first part of the string
//  (without the '$')

    result =
        parse(test_string2,
            (*(anychar_p - '$'))[refactor_action_actor(str)] >> '$'
        );

    if (result.full && str == std::string(test_string2))
    {
        cout << "Successfully parsed test string!" << endl;
        cout << "Parsed: \"" << str << "\"" << endl;
    }
    else
    {
        cout
            << "Correctly failed parsing the test string (without refactoring)!"
            << endl;
        cout << "Parsed instead: \"" << str << "\"" << endl;
    }
    cout << endl;

///////////////////////////////////////////////////////////////////////////////
//
//  3. Testing the refactor_action_d parser with an embedded (nested)
//  refactor_unary_p parser
//
//  The following test should successfully parse the test string, because the
//
//      refactor_action_unary_d[
//          ((*anychar_p)[refactor_action_actor(str)] - '$')
//      ] >> '$'
//
//  is refactored into
//
//      (*(anychar_p - '$'))[refactor_action_actor(str)] >> '$'.
//
///////////////////////////////////////////////////////////////////////////////

    const refactor_action_gen<refactor_unary_gen<> > refactor_action_unary_d =
        refactor_action_gen<refactor_unary_gen<> >(refactor_unary_d);

    result =
        parse(test_string2,
            refactor_action_unary_d[
                ((*anychar_p)[refactor_action_actor(str)] - '$')
            ] >> '$'
        );

    if (result.full)
    {
        cout
            << "Successfully refactored an action attached to an unary!"
            << endl;
        cout << "Parsed: \"" << str << "\"" << endl;
    }
    else
    {
        cout << "Failed to refactor an action!" << endl;
    }

//  Parsing the same test string without refactoring fails, because the
//  anychar_p eats up all the input up to the end of the string

    result =
        parse(test_string2,
            ((*anychar_p)[refactor_action_actor(str)] - '$') >> '$'
        );

    if (result.full)
    {
        cout << "Successfully parsed test string!" << endl;
        cout << "Parsed: \"" << str << "\"" << endl;
    }
    else
    {
        cout
            << "Correctly failed parsing the test string (without refactoring)!"
            << endl;
        cout << "Parsed instead: \"" << str << "\"" << endl;
    }
    cout << endl;

    return 0;
}

