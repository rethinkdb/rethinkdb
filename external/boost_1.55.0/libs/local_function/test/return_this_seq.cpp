
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/local_function.hpp>
#include <boost/function.hpp>
#include <boost/typeof/typeof.hpp>
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
#include <boost/detail/lightweight_test.hpp>

struct number;
BOOST_TYPEOF_REGISTER_TYPE(number) // Register before `bind this_` below.

struct number {
    number(int value) : value_(value) {}

    boost::function<int (void)> inc(void) {
        int BOOST_LOCAL_FUNCTION( (bind this_) ) {
            return ++this_->value_;
        } BOOST_LOCAL_FUNCTION_NAME(i)
        return i;
    }

private:
    int value_;
};

int main(void) {
    number n1 = 0; // Object valid in scope where closure is used.
    boost::function<int (void)> inc1 = n1.inc();
    number n2 = 0;
    boost::function<int (void)> inc2 = n2.inc();

    BOOST_TEST(inc1() == 1);
    BOOST_TEST(inc1() == 2);
    BOOST_TEST(inc2() == 1);
    BOOST_TEST(inc1() == 3);
    return boost::report_errors();
}

