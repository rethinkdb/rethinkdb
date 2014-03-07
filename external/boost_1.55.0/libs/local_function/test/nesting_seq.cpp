
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/local_function.hpp>
#include <boost/detail/lightweight_test.hpp>

int main(void) {
    int x = 0;

    void BOOST_LOCAL_FUNCTION( (bind& x) ) {
        void BOOST_LOCAL_FUNCTION( (bind& x) ) {
            x++;
        } BOOST_LOCAL_FUNCTION_NAME(g)

        x--;
        g();
    } BOOST_LOCAL_FUNCTION_NAME(f)
    
    f();

    BOOST_TEST(x == 0);
    return boost::report_errors();
}

