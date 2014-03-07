
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/functional/overloaded_function

#include "identity.hpp"
#include <boost/functional/overloaded_function.hpp>
#include <boost/detail/lightweight_test.hpp>

//[identity_make_checks
template<typename F>
void check(F identity) {
    BOOST_TEST(identity("abc") == "abc");
    BOOST_TEST(identity(123) == 123);
    BOOST_TEST(identity(1.23) == 1.23);
}
//]

int main() {
    //[identity_make_call
    check(boost::make_overloaded_function(identity_s, identity_i, identity_d));
    //]
    return boost::report_errors();
}

