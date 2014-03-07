/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <string>
#include <algorithm>

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <boost/fusion/include/io.hpp>

using namespace boost::phoenix;
using namespace boost::phoenix::arg_names;
using namespace std;

int
main()
{
    int     i100 = 100;
    string hello = "hello";
    const char* world = " world";

    int init[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    vector<int> v(init, init+10);

    char const* msg = "cout assert\n";
    (cout << arg1)(msg);
    (cout << arg1 << endl)(hello);
    (arg1 << hex)(cout);
    (cout << val(hello))();

    (cout << val(hello) << world << ", you da man!\n")();
    for_each(v.begin(), v.end(), cout << arg1 << ',');
    (cout << arg1 + 1)(i100);

    (cout << arg1 << "this is it, shukz:" << hex << arg2 << endl << endl)(msg, i100);

    int in;
    int out = 12345;
    stringstream sstr;
    (sstr << arg1)(out);
    (sstr >> arg1)(in);
    BOOST_TEST(in == out);

    return boost::report_errors();
}
