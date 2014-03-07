
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/config.hpp>
#ifdef BOOST_NO_CXX11_VARIADIC_MACROS
#   error "variadic macros required"
#else

#include <boost/local_function.hpp>
#include <boost/function.hpp>
#include <boost/detail/lightweight_test.hpp>

boost::function<int (void)> inc(int& value) {
    int BOOST_LOCAL_FUNCTION(bind& value) {
        return ++value;
    } BOOST_LOCAL_FUNCTION_NAME(i)
    return i;
}

int main(void) {
    int value1 = 0; // Reference valid in scope where closure is used.
    boost::function<int (void)> inc1 = inc(value1);
    int value2 = 0;
    boost::function<int (void)> inc2 = inc(value2);

    BOOST_TEST(inc1() == 1);
    BOOST_TEST(inc1() == 2);
    BOOST_TEST(inc2() == 1);
    BOOST_TEST(inc1() == 3);
    return boost::report_errors();
}

#endif // VARIADIC_MACROS

