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
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>

namespace phoenix = boost::phoenix;

int
main()
{
    using phoenix::val;
    using phoenix::arg_names::arg1;
    using phoenix::arg_names::arg2;
    using std::cout;
    using std::endl;
    using std::hex;
    using std::for_each;
    using std::string;
    using std::stringstream;
    using std::vector;

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
