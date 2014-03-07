
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/local_function.hpp>
#include <boost/detail/lightweight_test.hpp>

int add_square(int a, int b) {
    int BOOST_LOCAL_FUNCTION(int z) {
        return z * z;
    } BOOST_LOCAL_FUNCTION_NAME(square)

    return square(a) + square(b);
}

int main(void) {
    BOOST_TEST(add_square(2, 4) == 20);
    return boost::report_errors();
}

