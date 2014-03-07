
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/local_function.hpp>
#include <boost/detail/lightweight_test.hpp>

int main(void) {
    //[ten_void
    int BOOST_LOCAL_FUNCTION(void) { // No parameter.
        return 10;
    } BOOST_LOCAL_FUNCTION_NAME(ten)

    BOOST_TEST(ten() == 10);
    //]
    return boost::report_errors();
}

