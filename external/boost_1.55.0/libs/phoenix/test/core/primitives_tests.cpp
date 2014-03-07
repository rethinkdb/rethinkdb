/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <string>

#include <boost/phoenix/core/argument.hpp>
#include <boost/phoenix/core/reference.hpp>
#include <boost/phoenix/core/value.hpp>
#include <boost/detail/lightweight_test.hpp>

using boost::phoenix::cref;
using boost::phoenix::ref;
using boost::phoenix::val;


int
main()
{
#ifndef BOOST_PHOENIX_NO_PREDEFINED_TERMINALS
    using boost::phoenix::arg_names::_1;
    using boost::phoenix::arg_names::arg1;
    using boost::phoenix::arg_names::arg2;
#else 
    boost::phoenix::arg_names::_1_type _1;
    boost::phoenix::arg_names::arg1_type arg1;
    boost::phoenix::arg_names::arg2_type arg2;
#endif
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
    val(' ')();
    val(" ")();
    std::cout << val("Hello,")() << val(' ')() << val("World")() << std::endl;
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
