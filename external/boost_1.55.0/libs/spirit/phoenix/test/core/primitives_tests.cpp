/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <string>

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/detail/lightweight_test.hpp>

using namespace boost::phoenix;
using namespace boost::phoenix::arg_names;
using namespace std;

int
main()
{
    char c1 = '1';
    int i1 = 1, i2 = 2, i = 4;
    const char* s2 = "2";

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Values, references and arguments
    //
    ///////////////////////////////////////////////////////////////////////////

    //  argument
    BOOST_TEST(arg1(c1) == c1);
    BOOST_TEST(arg1(i1, i2) == i1);
    BOOST_TEST(arg2(i1, s2) == s2);
    BOOST_TEST(&(arg1(c1)) == &c1); // must be an lvalue

    //  value
    cout << val("Hello,")() << val(' ')() << val("World")() << endl;
    BOOST_TEST(val(3)() == 3);
    BOOST_TEST(val("Hello, world")() == std::string("Hello, world"));
    BOOST_TEST(val(_1)(i1) == i1);

    //  should not compile:
#ifdef PHOENIX_SHOULD_NOT_COMPILE_TEST
    &val(_1)(i1); // should return an rvalue
#endif

    //  reference
    BOOST_TEST(cref(i)() == ref(i)());
    BOOST_TEST(cref(i)() == 4);
    BOOST_TEST(i == 4);
    BOOST_TEST(ref(++i)() == 5);
    BOOST_TEST(i == 5);    

    //  should not compile:
#ifdef PHOENIX_SHOULD_NOT_COMPILE_TEST
    ref(arg1);
#endif

    // testing consts
    int const ic = 123;
    BOOST_TEST(arg1(ic) == 123);

    //  should not compile:
#ifdef PHOENIX_SHOULD_NOT_COMPILE_TEST
    arg1();
#endif

    return boost::report_errors();
}
