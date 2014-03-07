
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

//[add_with_default_macro
#define WITH_DEFAULT , default
//]

int main(void) {
    //[add_with_default
    int BOOST_LOCAL_FUNCTION(int x, int y WITH_DEFAULT 2) { // Default.
        return x + y;
    } BOOST_LOCAL_FUNCTION_NAME(add)

    BOOST_TEST(add(1) == 3);
    //]
    return boost::report_errors();
}

#endif // VARIADIC_MACROS

