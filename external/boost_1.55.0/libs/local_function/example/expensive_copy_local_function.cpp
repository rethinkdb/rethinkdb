
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/local_function.hpp>
#include <boost/typeof/typeof.hpp>
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
#include <iostream>
#include <cassert>

//[expensive_copy_local_function
struct n {
    int i;
    n(int _i): i(_i) {}
    n(n const& x): i(x.i) { // Some time consuming copy operation.
        for (unsigned i = 0; i < 10000; ++i) std::cout << '.';
    }
};
BOOST_TYPEOF_REGISTER_TYPE(n) // Register for `bind& x` below.

int main(void) {
    n x(-1);

    void BOOST_LOCAL_FUNCTION(const bind& x) {  // OK: No copy expensive
        assert(x.i == -1);                      // copy but constant.
    } BOOST_LOCAL_FUNCTION_NAME(f)
    f();

    return 0;
}
//]

