///////////////////////////////////////////////////////////////////////////////
// mem_ptr.hpp
//
//  Copyright 2009 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/mpl/print.hpp>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/proto/proto.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/test/unit_test.hpp>

namespace proto = boost::proto;
using proto::_;

struct evaluator
  : proto::when<_, proto::_default<evaluator> >
{};

template<typename Ret, typename Expr>
void assert_result_type(Expr &)
{
    // check that the return type as calculated by the _default transform
    // is correct.
    BOOST_MPL_ASSERT((
        boost::is_same<
            Ret
          , typename boost::result_of<evaluator(Expr &)>::type
        >
    ));

    // check that the return type as calculated by the default_context
    // is correct.
    BOOST_MPL_ASSERT((
        boost::is_same<
            Ret
          , typename boost::result_of<proto::functional::eval(Expr &, proto::default_context &)>::type
        >
    ));
}

///////////////////////////////////////////////////////////////////////////////
struct S
{
    S() : x(-42) {}
    int x;
};

// like a normal terminal except with an operator() that can
// accept non-const lvalues (Proto's only accepts const lvalues)
template<typename T, typename Dummy = proto::is_proto_expr>
struct my_terminal
{
    typedef typename proto::terminal<T>::type my_terminal_base;
    BOOST_PROTO_BASIC_EXTENDS(my_terminal_base, my_terminal, proto::default_domain)

    template<typename A0>
    typename proto::result_of::make_expr<proto::tag::function, my_terminal const &, A0 &>::type const
    operator()(A0 &a0) const
    {
        return proto::make_expr<proto::tag::function>(boost::cref(*this), boost::ref(a0));
    }

    template<typename A0>
    typename proto::result_of::make_expr<proto::tag::function, my_terminal const &, A0 const &>::type const
    operator()(A0 const &a0) const
    {
        return proto::make_expr<proto::tag::function>(boost::cref(*this), boost::cref(a0));
    }
};

my_terminal<int S::*> test1 = {{ &S::x }};

// Some tests with the default transform
void test_refs_transform()
{
    S s;
    BOOST_REQUIRE_EQUAL(s.x, -42);

    // Check that evaluating a memptr invocation with a
    // non-const lvalue argument yields the member as a
    // non-const lvalue
    assert_result_type<int &>(test1(s));
    evaluator()(test1(s)) = 0;
    BOOST_CHECK_EQUAL(s.x, 0);

    // Ditto for reference_wrappers
    assert_result_type<int &>(test1(boost::ref(s)));
    evaluator()(test1(boost::ref(s))) = 42;
    BOOST_CHECK_EQUAL(s.x, 42);

    // Check that evaluating a memptr invocation with a
    // const lvalue argument yields the member as a
    // const lvalue
    S const &rcs = s;
    assert_result_type<int const &>(test1(rcs));
    int const &s_x = evaluator()(test1(rcs));
    BOOST_CHECK_EQUAL(&s.x, &s_x);
}

// Some tests with the default context
void test_refs_context()
{
    proto::default_context ctx;
    S s;
    BOOST_REQUIRE_EQUAL(s.x, -42);

    // Check that evaluating a memptr invocation with a
    // non-const lvalue argument yields the member as a
    // non-const lvalue
    assert_result_type<int &>(test1(s));
    proto::eval(test1(s), ctx) = 0;
    BOOST_CHECK_EQUAL(s.x, 0);

    // Ditto for reference_wrappers
    assert_result_type<int &>(test1(boost::ref(s)));
    proto::eval(test1(boost::ref(s)), ctx) = 42;
    BOOST_CHECK_EQUAL(s.x, 42);

    // Check that evaluating a memptr invocation with a
    // const lvalue argument yields the member as a
    // const lvalue
    S const &rcs = s;
    assert_result_type<int const &>(test1(rcs));
    int const &s_x = proto::eval(test1(rcs), ctx);
    BOOST_CHECK_EQUAL(&s.x, &s_x);
}

void test_ptrs_transform()
{
    S s;
    BOOST_REQUIRE_EQUAL(s.x, -42);

    // Check that evaluating a memptr invocation with a
    // pointer to a non-const argument yields the member as a
    // non-const lvalue
    assert_result_type<int &>(test1(&s));
    evaluator()(test1(&s)) = 0;
    BOOST_CHECK_EQUAL(s.x, 0);

    S* ps = &s;
    assert_result_type<int &>(test1(ps));
    evaluator()(test1(ps)) = 42;
    BOOST_CHECK_EQUAL(s.x, 42);

    boost::shared_ptr<S> const sp(new S);
    BOOST_REQUIRE_EQUAL(sp->x, -42);

    // Ditto for shared_ptr (which hook the get_pointer()
    // customization point)
    assert_result_type<int &>(test1(sp));
    evaluator()(test1(sp)) = 0;
    BOOST_CHECK_EQUAL(sp->x, 0);

    // Check that evaluating a memptr invocation with a
    // const lvalue argument yields the member as a
    // const lvalue
    S const &rcs = s;
    assert_result_type<int const &>(test1(&rcs));
    int const &s_x0 = evaluator()(test1(&rcs));
    BOOST_CHECK_EQUAL(&s.x, &s_x0);

    S const *pcs = &s;
    assert_result_type<int const &>(test1(pcs));
    int const &s_x1 = evaluator()(test1(pcs));
    BOOST_CHECK_EQUAL(&s.x, &s_x1);

    boost::shared_ptr<S const> spc(new S);
    BOOST_REQUIRE_EQUAL(spc->x, -42);

    assert_result_type<int const &>(test1(spc));
    int const &s_x2 = evaluator()(test1(spc));
    BOOST_CHECK_EQUAL(&spc->x, &s_x2);
}

void test_ptrs_context()
{
    proto::default_context ctx;
    S s;
    BOOST_REQUIRE_EQUAL(s.x, -42);

    // Check that evaluating a memptr invocation with a
    // pointer to a non-const argument yields the member as a
    // non-const lvalue
    assert_result_type<int &>(test1(&s));
    proto::eval(test1(&s), ctx) = 0;
    BOOST_CHECK_EQUAL(s.x, 0);

    S* ps = &s;
    assert_result_type<int &>(test1(ps));
    proto::eval(test1(ps), ctx) = 42;
    BOOST_CHECK_EQUAL(s.x, 42);

    boost::shared_ptr<S> const sp(new S);
    BOOST_REQUIRE_EQUAL(sp->x, -42);

    // Ditto for shared_ptr (which hook the get_pointer()
    // customization point)
    assert_result_type<int &>(test1(sp));
    proto::eval(test1(sp), ctx) = 0;
    BOOST_CHECK_EQUAL(sp->x, 0);

    // Check that evaluating a memptr invocation with a
    // const lvalue argument yields the member as a
    // const lvalue
    S const &rcs = s;
    assert_result_type<int const &>(test1(&rcs));
    int const &s_x0 = proto::eval(test1(&rcs), ctx);
    BOOST_CHECK_EQUAL(&s.x, &s_x0);

    S const *pcs = &s;
    assert_result_type<int const &>(test1(pcs));
    int const &s_x1 = proto::eval(test1(pcs), ctx);
    BOOST_CHECK_EQUAL(&s.x, &s_x1);

    boost::shared_ptr<S const> spc(new S);
    BOOST_REQUIRE_EQUAL(spc->x, -42);

    assert_result_type<int const &>(test1(spc));
    int const &s_x2 = proto::eval(test1(spc), ctx);
    BOOST_CHECK_EQUAL(&spc->x, &s_x2);
}

///////////////////////////////////////////////////////////////////////////////
struct T
{
    int x;
};

proto::terminal<int T::*>::type test2 = { &T::x };

int const *get_pointer(T const &t)
{
    return &t.x;
}

void with_get_pointer_transform()
{
    T t;
    evaluator()(test2(t));
}

///////////////////////////////////////////////////////////////////////////////
template<typename T>
struct dumb_ptr
{
    dumb_ptr(T *p_) : p(p_) {}

    friend T const *get_pointer(dumb_ptr const &p)
    {
        return p.p;
    }

    T *p;
};

struct U : dumb_ptr<U>
{
    U() : dumb_ptr<U>(this), x(42) {}
    int x;
};

my_terminal<U *dumb_ptr<U>::*> U_p = {{&U::p}};
my_terminal<int U::*> U_x = {{&U::x}};

void potentially_ambiguous_transform()
{
    U u;

    // This should yield a non-const reference to a pointer-to-const
    U *&up = evaluator()(U_p(u));
    BOOST_CHECK_EQUAL(&up, &u.p);

    // This should yield a non-const reference to a pointer-to-const
    int &ux = evaluator()(U_x(u));
    BOOST_CHECK_EQUAL(&ux, &u.x);
}


using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test handling of member pointers by the default transform and default contexts");

    test->add(BOOST_TEST_CASE(&test_refs_transform));
    test->add(BOOST_TEST_CASE(&test_refs_context));

    test->add(BOOST_TEST_CASE(&test_ptrs_transform));
    test->add(BOOST_TEST_CASE(&test_ptrs_context));

    test->add(BOOST_TEST_CASE(&with_get_pointer_transform));

    test->add(BOOST_TEST_CASE(&potentially_ambiguous_transform));

    return test;
}
