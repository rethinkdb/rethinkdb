// Boost.TypeErasure library
//
// Copyright 2012 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_free.cpp 83332 2013-03-06 18:46:56Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/free.hpp>
#include <boost/mpl/vector.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::type_erasure;

struct model {
    explicit model(int v) : val(v) {}
    int val;
};

int f1(model& m) { return m.val; }
int f1(model& m, int i) { return m.val + i; }

BOOST_TYPE_ERASURE_FREE((global_has_f1_1), f1, 1);

BOOST_AUTO_TEST_CASE(test_global_has_f1_1) {
    typedef ::boost::mpl::vector<
        global_has_f1_1<int(_self&)>,
        copy_constructible<> > concept_type;
    model m(10);
    any<concept_type> x(m);
    BOOST_CHECK_EQUAL(f1(x), 10);
}

BOOST_TYPE_ERASURE_FREE((ns1)(ns2)(ns_has_f1_1), f1, 1);

BOOST_AUTO_TEST_CASE(test_ns_has_f1_1) {
    typedef ::boost::mpl::vector<
        ns1::ns2::ns_has_f1_1<int(_self&)>,
        copy_constructible<> > concept_type;
    model m(10);
    any<concept_type> x(m);
    BOOST_CHECK_EQUAL(f1(x), 10);
}

struct model_const {
    explicit model_const(int v) : val(v) {}
    int val;
};

int f1(const model_const& m) { return m.val; }
int f1(const model_const& m, int i) { return m.val + i; }

BOOST_AUTO_TEST_CASE(test_global_has_f1_1_const) {
    typedef ::boost::mpl::vector<
        ns1::ns2::ns_has_f1_1<int(const _self&)>,
        copy_constructible<> > concept_type;
    model_const m(10);
    const any<concept_type> x(m);
    BOOST_CHECK_EQUAL(f1(x), 10);
}

BOOST_AUTO_TEST_CASE(test_global_has_f1_1_void) {
    typedef ::boost::mpl::vector<
        global_has_f1_1<void(_self&)>,
        copy_constructible<> > concept_type;
    model m(10);
    any<concept_type> x(m);
    f1(x);
}

BOOST_TYPE_ERASURE_FREE((global_has_f1_2), f1, 2);

BOOST_AUTO_TEST_CASE(test_global_has_f1_overload) {
    typedef ::boost::mpl::vector<
        global_has_f1_1<int(_self&)>,
        global_has_f1_2<int(_self&, int)>,
        copy_constructible<> > concept_type;
    model m(10);
    any<concept_type> x(m);
    BOOST_CHECK_EQUAL(f1(x), 10);
    BOOST_CHECK_EQUAL(f1(x, 5), 15);
}

BOOST_AUTO_TEST_CASE(test_global_has_f1_overload_const) {
    typedef ::boost::mpl::vector<
        global_has_f1_1<int(const _self&)>,
        global_has_f1_2<int(const _self&, int)>,
        copy_constructible<> > concept_type;
    model_const m(10);
    const any<concept_type> x(m);
    BOOST_CHECK_EQUAL(f1(x), 10);
    BOOST_CHECK_EQUAL(f1(x, 5), 15);
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

BOOST_AUTO_TEST_CASE(test_global_has_f1_rv) {
    typedef ::boost::mpl::vector<
        global_has_f1_2<int(_self&&, int&&)>,
        copy_constructible<> > concept_type;
    model_const m(10);
    any<concept_type> x(m);
    BOOST_CHECK_EQUAL(f1(std::move(x), 5), 15);
}

#endif
