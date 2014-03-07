
// Copyright (C) 2006-2009, 2012 Alexander Nasonov
// Copyright (C) 2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/scope_exit

#include <boost/scope_exit.hpp>
#include <boost/config.hpp>
#include <boost/typeof/typeof.hpp>
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
#include <boost/detail/lightweight_test.hpp>

template<typename T>
struct this_tester;
BOOST_TYPEOF_REGISTER_TEMPLATE(this_tester, 1) // Before`this_` capture.

template<typename T>
struct this_tester {
    void check(void) {
        value_ = -1;

        BOOST_SCOPE_EXIT_TPL( (this_) ) {
            BOOST_TEST(this_->value_ == 0);
        } BOOST_SCOPE_EXIT_END

#ifndef BOOST_NO_CXX11_LAMBDAS
        BOOST_SCOPE_EXIT_ALL(&, this) {
            BOOST_TEST(this->value_ == 0);
        };
#endif // lambdas

        value_ = 0;
    }

private:
    T value_;
};

int main(void) {
    this_tester<int>().check();
    return boost::report_errors();
}

