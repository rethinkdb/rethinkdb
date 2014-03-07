//  result_of_tests.cpp  -- The Boost Lambda Library ------------------
//
// Copyright (C) 2010 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org

// -----------------------------------------------------------------------


#include <boost/test/minimal.hpp>    // see "Header Implementation Option"
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

struct with_result_type {
    typedef int result_type;
    int operator()() const { return 0; }
    int operator()(int) const { return 1; }
    int operator()(int, int) const { return 2; }
    int operator()(int, int, int) const { return 3; }
    int operator()(int, int, int, int) const { return 4; }
    int operator()(int, int, int, int, int) const { return 5; }
    int operator()(int, int, int, int, int, int) const { return 6; }
    int operator()(int, int, int, int, int, int, int) const { return 7; }
    int operator()(int, int, int, int, int, int, int, int) const { return 8; }
    int operator()(int, int, int, int, int, int, int, int, int) const { return 9; }
};

struct with_result_template_value {
    template<class Sig>
    struct result;
    template<class This>
    struct result<This()> {
        typedef int type;
    };
    template<class This, class A1>
    struct result<This(A1)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int>));
        typedef int type;
    };
    template<class This, class A1, class A2>
    struct result<This(A1, A2)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int>));
        BOOST_MPL_ASSERT((boost::is_same<A2, int>));
        typedef int type;
    };
    template<class This, class A1, class A2, class A3>
    struct result<This(A1, A2, A3)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int>));
        BOOST_MPL_ASSERT((boost::is_same<A2, int>));
        BOOST_MPL_ASSERT((boost::is_same<A3, int>));
        typedef int type;
    };
    template<class This, class A1, class A2, class A3, class A4>
    struct result<This(A1, A2, A3, A4)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int>));
        BOOST_MPL_ASSERT((boost::is_same<A2, int>));
        BOOST_MPL_ASSERT((boost::is_same<A3, int>));
        BOOST_MPL_ASSERT((boost::is_same<A4, int>));
        typedef int type;
    };
    template<class This, class A1, class A2, class A3, class A4, class A5>
    struct result<This(A1, A2, A3, A4, A5)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int>));
        BOOST_MPL_ASSERT((boost::is_same<A2, int>));
        BOOST_MPL_ASSERT((boost::is_same<A3, int>));
        BOOST_MPL_ASSERT((boost::is_same<A4, int>));
        BOOST_MPL_ASSERT((boost::is_same<A5, int>));
        typedef int type;
    };
    template<class This, class A1, class A2, class A3, class A4, class A5, class A6>
    struct result<This(A1, A2, A3, A4, A5, A6)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int>));
        BOOST_MPL_ASSERT((boost::is_same<A2, int>));
        BOOST_MPL_ASSERT((boost::is_same<A3, int>));
        BOOST_MPL_ASSERT((boost::is_same<A4, int>));
        BOOST_MPL_ASSERT((boost::is_same<A5, int>));
        BOOST_MPL_ASSERT((boost::is_same<A6, int>));
        typedef int type;
    };
    template<class This, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
    struct result<This(A1, A2, A3, A4, A5, A6, A7)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int>));
        BOOST_MPL_ASSERT((boost::is_same<A2, int>));
        BOOST_MPL_ASSERT((boost::is_same<A3, int>));
        BOOST_MPL_ASSERT((boost::is_same<A4, int>));
        BOOST_MPL_ASSERT((boost::is_same<A5, int>));
        BOOST_MPL_ASSERT((boost::is_same<A6, int>));
        BOOST_MPL_ASSERT((boost::is_same<A7, int>));
        typedef int type;
    };
    template<class This, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
    struct result<This(A1, A2, A3, A4, A5, A6, A7, A8)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int>));
        BOOST_MPL_ASSERT((boost::is_same<A2, int>));
        BOOST_MPL_ASSERT((boost::is_same<A3, int>));
        BOOST_MPL_ASSERT((boost::is_same<A4, int>));
        BOOST_MPL_ASSERT((boost::is_same<A5, int>));
        BOOST_MPL_ASSERT((boost::is_same<A6, int>));
        BOOST_MPL_ASSERT((boost::is_same<A7, int>));
        BOOST_MPL_ASSERT((boost::is_same<A8, int>));
        typedef int type;
    };
    template<class This, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
    struct result<This(A1, A2, A3, A4, A5, A6, A7, A8, A9)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int>));
        BOOST_MPL_ASSERT((boost::is_same<A2, int>));
        BOOST_MPL_ASSERT((boost::is_same<A3, int>));
        BOOST_MPL_ASSERT((boost::is_same<A4, int>));
        BOOST_MPL_ASSERT((boost::is_same<A5, int>));
        BOOST_MPL_ASSERT((boost::is_same<A6, int>));
        BOOST_MPL_ASSERT((boost::is_same<A7, int>));
        BOOST_MPL_ASSERT((boost::is_same<A8, int>));
        BOOST_MPL_ASSERT((boost::is_same<A9, int>));
        typedef int type;
    };

    int operator()() const { return 0; }
    int operator()(int) const { return 1; }
    int operator()(int, int) const { return 2; }
    int operator()(int, int, int) const { return 3; }
    int operator()(int, int, int, int) const { return 4; }
    int operator()(int, int, int, int, int) const { return 5; }
    int operator()(int, int, int, int, int, int) const { return 6; }
    int operator()(int, int, int, int, int, int, int) const { return 7; }
    int operator()(int, int, int, int, int, int, int, int) const { return 8; }
    int operator()(int, int, int, int, int, int, int, int, int) const { return 9; }
};

struct with_result_template_reference {
    template<class Sig>
    struct result;
    template<class This>
    struct result<This()> {
        typedef int type;
    };
    template<class This, class A1>
    struct result<This(A1)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int&>));
        typedef int type;
    };
    template<class This, class A1, class A2>
    struct result<This(A1, A2)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A2, int&>));
        typedef int type;
    };
    template<class This, class A1, class A2, class A3>
    struct result<This(A1, A2, A3)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A2, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A3, int&>));
        typedef int type;
    };
    template<class This, class A1, class A2, class A3, class A4>
    struct result<This(A1, A2, A3, A4)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A2, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A3, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A4, int&>));
        typedef int type;
    };
    template<class This, class A1, class A2, class A3, class A4, class A5>
    struct result<This(A1, A2, A3, A4, A5)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A2, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A3, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A4, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A5, int&>));
        typedef int type;
    };
    template<class This, class A1, class A2, class A3, class A4, class A5, class A6>
    struct result<This(A1, A2, A3, A4, A5, A6)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A2, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A3, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A4, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A5, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A6, int&>));
        typedef int type;
    };
    template<class This, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
    struct result<This(A1, A2, A3, A4, A5, A6, A7)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A2, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A3, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A4, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A5, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A6, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A7, int&>));
        typedef int type;
    };
    template<class This, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
    struct result<This(A1, A2, A3, A4, A5, A6, A7, A8)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A2, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A3, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A4, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A5, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A6, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A7, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A8, int&>));
        typedef int type;
    };
    template<class This, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
    struct result<This(A1, A2, A3, A4, A5, A6, A7, A8, A9)> {
        BOOST_MPL_ASSERT((boost::is_same<A1, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A2, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A3, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A4, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A5, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A6, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A7, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A8, int&>));
        BOOST_MPL_ASSERT((boost::is_same<A9, int&>));
        typedef int type;
    };

    int operator()() const { return 0; }
    int operator()(int) const { return 1; }
    int operator()(int, int) const { return 2; }
    int operator()(int, int, int) const { return 3; }
    int operator()(int, int, int, int) const { return 4; }
    int operator()(int, int, int, int, int) const { return 5; }
    int operator()(int, int, int, int, int, int) const { return 6; }
    int operator()(int, int, int, int, int, int, int) const { return 7; }
    int operator()(int, int, int, int, int, int, int, int) const { return 8; }
    int operator()(int, int, int, int, int, int, int, int, int) const { return 9; }
};

template<class F>
typename boost::result_of<F()>::type apply0(F f) {
    return f();
}
template<class A, class F>
typename boost::result_of<F(A)>::type apply1(F f, A a) {
    return f(a);
}
template<class A, class B, class F>
typename boost::result_of<F(A, B)>::type apply2(F f, A a, B b) {
    return f(a, b);
}
template<class A, class B, class C, class F>
typename boost::result_of<F(A, B, C)>::type apply3(F f, A a, B b, C c) {
    return f(a, b, c);
}

using namespace boost::lambda;

int test_main(int, char *[]) {
    BOOST_CHECK(boost::lambda::bind(with_result_type())() == 0);
    BOOST_CHECK(boost::lambda::bind(with_result_type(), 1)() == 1);
    BOOST_CHECK(boost::lambda::bind(with_result_type(), 1, 2)() == 2);
    BOOST_CHECK(boost::lambda::bind(with_result_type(), 1, 2, 3)() == 3);
    BOOST_CHECK(boost::lambda::bind(with_result_type(), 1, 2, 3, 4)() == 4);
    BOOST_CHECK(boost::lambda::bind(with_result_type(), 1, 2, 3, 4, 5)() == 5);
    BOOST_CHECK(boost::lambda::bind(with_result_type(), 1, 2, 3, 4, 5, 6)() == 6);
    BOOST_CHECK(boost::lambda::bind(with_result_type(), 1, 2, 3, 4, 5, 6, 7)() == 7);
    BOOST_CHECK(boost::lambda::bind(with_result_type(), 1, 2, 3, 4, 5, 6, 7, 8)() == 8);
    BOOST_CHECK(boost::lambda::bind(with_result_type(), 1, 2, 3, 4, 5, 6, 7, 8, 9)() == 9);
    
    // Nullary result_of fails
    //BOOST_CHECK(boost::lambda::bind(with_result_template_value())() == 0);
    BOOST_CHECK(boost::lambda::bind(with_result_template_value(), 1)() == 1);
    BOOST_CHECK(boost::lambda::bind(with_result_template_value(), 1, 2)() == 2);
    BOOST_CHECK(boost::lambda::bind(with_result_template_value(), 1, 2, 3)() == 3);
    BOOST_CHECK(boost::lambda::bind(with_result_template_value(), 1, 2, 3, 4)() == 4);
    BOOST_CHECK(boost::lambda::bind(with_result_template_value(), 1, 2, 3, 4, 5)() == 5);
    BOOST_CHECK(boost::lambda::bind(with_result_template_value(), 1, 2, 3, 4, 5, 6)() == 6);
    BOOST_CHECK(boost::lambda::bind(with_result_template_value(), 1, 2, 3, 4, 5, 6, 7)() == 7);
    BOOST_CHECK(boost::lambda::bind(with_result_template_value(), 1, 2, 3, 4, 5, 6, 7, 8)() == 8);
    BOOST_CHECK(boost::lambda::bind(with_result_template_value(), 1, 2, 3, 4, 5, 6, 7, 8, 9)() == 9);

    int one = 1,
        two = 2,
        three = 3,
        four = 4,
        five = 5,
        six = 6,
        seven = 7,
        eight = 8,
        nine = 9;

    // Nullary result_of fails
    //BOOST_CHECK(boost::lambda::bind(with_result_template_reference())() == 0);
    BOOST_CHECK(boost::lambda::bind(with_result_template_reference(), var(one))() == 1);
    BOOST_CHECK(boost::lambda::bind(with_result_template_reference(), var(one), var(two))() == 2);
    BOOST_CHECK(boost::lambda::bind(with_result_template_reference(), var(one), var(two), var(three))() == 3);
    BOOST_CHECK(boost::lambda::bind(with_result_template_reference(), var(one), var(two), var(three), var(four))() == 4);
    BOOST_CHECK(boost::lambda::bind(with_result_template_reference(), var(one), var(two), var(three), var(four), var(five))() == 5);
    BOOST_CHECK(boost::lambda::bind(with_result_template_reference(), var(one), var(two), var(three), var(four), var(five), var(six))() == 6);
    BOOST_CHECK(boost::lambda::bind(with_result_template_reference(), var(one), var(two), var(three), var(four), var(five), var(six), var(seven))() == 7);
    BOOST_CHECK(boost::lambda::bind(with_result_template_reference(), var(one), var(two), var(three), var(four), var(five), var(six), var(seven), var(eight))() == 8);
    BOOST_CHECK(boost::lambda::bind(with_result_template_reference(), var(one), var(two), var(three), var(four), var(five), var(six), var(seven), var(eight), var(nine))() == 9);

    // Check using result_of with lambda functors
    //BOOST_CHECK(apply0(constant(0)) == 0);
    BOOST_CHECK(apply1<int>(_1, one) == 1);
    BOOST_CHECK(apply1<int&>(_1, one) == 1);
    BOOST_CHECK(apply1<const int&>(_1, one) == 1);
    BOOST_CHECK((apply2<int, int>(_1 + _2, one, two) == 3));
    BOOST_CHECK((apply2<int&, int&>(_1 + _2, one, two) == 3));
    BOOST_CHECK((apply2<const int&, const int&>(_1 + _2, one, two) == 3));
    BOOST_CHECK((apply3<int, int, int>(_1 + _2 + _3, one, two, three) == 6));
    BOOST_CHECK((apply3<int&, int&, int&>(_1 + _2 + _3, one, two, three) == 6));
    BOOST_CHECK((apply3<const int&, const int&, const int&>(_1 + _2 + _3, one, two, three) == 6));

    return 0;
}
