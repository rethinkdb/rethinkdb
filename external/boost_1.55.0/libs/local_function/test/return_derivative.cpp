
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
#include <boost/typeof/typeof.hpp>
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
#include <boost/detail/lightweight_test.hpp>

BOOST_TYPEOF_REGISTER_TEMPLATE(boost::function, 1)

boost::function<int (int)> derivative(boost::function<int (int)>& f, int dx) {
    int BOOST_LOCAL_FUNCTION(bind& f, const bind dx, int x) {
        return (f(x + dx) - f(x)) / dx;
    } BOOST_LOCAL_FUNCTION_NAME(deriv)

    return deriv;
}

int main(void) {
    int BOOST_LOCAL_FUNCTION(int x) {
        return x + 4;
    } BOOST_LOCAL_FUNCTION_NAME(add2)
    
    boost::function<int (int)> a2 = add2; // Reference valid where closure used.
    boost::function<int (int)> d2 = derivative(a2, 2);
    BOOST_TEST(d2(6) == 1);
    return boost::report_errors();
}

#endif // VARIADIC_MACROS

