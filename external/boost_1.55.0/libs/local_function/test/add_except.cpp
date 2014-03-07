
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
#include <boost/detail/lightweight_test.hpp>

int main(void) {
    //[add_except
    double sum = 0.0;
    int factor = 10;

    void BOOST_LOCAL_FUNCTION(const bind factor, bind& sum,
            double num) throw() { // Throw nothing.
        sum += factor * num;
    } BOOST_LOCAL_FUNCTION_NAME(add)

    add(100);
    //]
    
    BOOST_TEST(sum == 1000);
    return boost::report_errors();
}

#endif // VARIADIC_MACROS

