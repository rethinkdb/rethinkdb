//  ret_test.cpp  - The Boost Lambda Library -----------------------
//
// Copyright (C) 2009 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org

#include <boost/test/minimal.hpp>

#include <boost/lambda/lambda.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

template<class R, class F>
void test_ret(R r, F f) {
    typename F::result_type x = f();
    BOOST_MPL_ASSERT((boost::is_same<R, typename F::result_type>));
    BOOST_CHECK(x == r);
}

template<class R, class F, class T1>
void test_ret(R r, F f, T1& t1) {
    typename F::result_type x = f(t1);
    BOOST_MPL_ASSERT((boost::is_same<R, typename F::result_type>));
    BOOST_CHECK(x == r);
}

class add_result {
public:
    add_result(int i = 0) : value(i) {}
    friend bool operator==(const add_result& lhs, const add_result& rhs) {
        return(lhs.value == rhs.value);
    }
private:
    int value;
};

class addable {};
add_result operator+(addable, addable) {
    return add_result(7);
}

int test_main(int, char*[]) {
    addable test;
    test_ret(add_result(7), boost::lambda::ret<add_result>(boost::lambda::_1 + test), test);
    test_ret(8.0, boost::lambda::ret<double>(boost::lambda::constant(7) + 1));

    return 0;
}
