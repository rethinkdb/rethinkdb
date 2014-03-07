
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/config.hpp>
#ifdef BOOST_NO_CXX11_LAMBDAS
#   error "lambda functions required"
#else

#include <boost/noncopyable.hpp>
#include <cassert>

//[noncopyable_cxx11_lambda_error
struct n: boost::noncopyable {
    int i;
    n(int _i): i(_i) {}
};


int main(void) {
    n x(-1);

    auto f = [x](void) {    // Error: x is non-copyable, but if
        assert(x.i == -1);  // bind `&x` then `x` is not constant.
    };
    f();

    return 0;
}
//]

#endif // LAMBDAS

