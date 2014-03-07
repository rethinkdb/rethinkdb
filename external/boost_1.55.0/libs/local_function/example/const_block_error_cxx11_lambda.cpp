
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/config.hpp>
#ifdef BOOST_NO_CXX11_LAMBDAS
#   error "requires lambda functions"
#else

#include <cassert>

int main(void) {
    //[const_block_cxx11_lambda
    int x = 1, y = 2;
    const decltype(x)& const_x = x; // Constant so cannot be modified
    const decltype(y)& const_y = y; // and reference so no copy.
    [&const_x, &const_y]() {        // Lambda functions (C++11 only).
        assert(const_x = const_y);  // Unfortunately, `const_` names.
    }();
    //]
    return 0;
}

#endif // LAMBDAS

