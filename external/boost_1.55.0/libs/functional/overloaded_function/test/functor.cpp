
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/functional/overloaded_function

#include "identity.hpp"
#include <boost/functional/overloaded_function.hpp>
#include <boost/detail/lightweight_test.hpp>

int main() {
    //[identity_calls
    BOOST_TEST(identity_s("abc") == "abc");
    BOOST_TEST(identity_i(123) == 123);
    BOOST_TEST(identity_d(1.23) == 1.23);
    //]

    //[identity_functor
    boost::overloaded_function<
          const std::string& (const std::string&)
        , int (int)
        , double (double)
    > identity(identity_s, identity_i, identity_d);

    // All calls via single `identity` function.
    BOOST_TEST(identity("abc") == "abc");
    BOOST_TEST(identity(123) == 123);
    BOOST_TEST(identity(1.23) == 1.23);
    //]

    return boost::report_errors();
}

