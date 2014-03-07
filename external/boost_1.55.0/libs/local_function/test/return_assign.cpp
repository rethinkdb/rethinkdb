
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
#include <iostream>

//[return_assign
void call1(boost::function<int (int) > f) { BOOST_TEST(f(1) == 5); }
void call0(boost::function<int (void)> f) { BOOST_TEST(f() == 5); }

boost::function<int (int, int)> linear(const int& slope) {
    int BOOST_LOCAL_FUNCTION(const bind& slope,
            int x, default 1, int y, default 2) {
        return x + slope * y;
    } BOOST_LOCAL_FUNCTION_NAME(lin)

    boost::function<int (int, int)> f = lin; // Assign to local variable.
    BOOST_TEST(f(1, 2) == 5);

    call1(lin); // Pass to other functions.
    call0(lin);

    return lin; // Return.
}

void call(void) {
    boost::function<int (int, int)> f = linear(2);
    BOOST_TEST(f(1, 2) == 5);
}
//]

int main(void) {
    call();
    return boost::report_errors();
}

#endif // VARIADIC_MACROS

