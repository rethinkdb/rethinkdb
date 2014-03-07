///////////////////////////////////////////////////////////////////////////////
// proto::make_expr.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <boost/proto/core.hpp>
#include <boost/proto/transform.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/fusion/tuple.hpp>
#include <boost/test/unit_test.hpp>

namespace fusion = boost::fusion;
namespace proto = boost::proto;

template<typename E> struct ewrap;

struct mydomain
  : proto::domain<proto::generator<ewrap> >
{};

template<typename E> struct ewrap
  : proto::extends<E, ewrap<E>, mydomain>
{
    explicit ewrap(E const &e = E())
      : proto::extends<E, ewrap<E>, mydomain>(e)
    {}
};

void test_make_expr()
{
    int i = 42;
    proto::terminal<int>::type t1 = proto::make_expr<proto::tag::terminal>(1);
    proto::terminal<int>::type t2 = proto::make_expr<proto::tag::terminal>(i);
    proto::unary_plus<proto::terminal<int>::type>::type p1 = proto::make_expr<proto::tag::unary_plus>(1);
    proto::unary_plus<proto::terminal<int>::type>::type p2 = proto::make_expr<proto::tag::unary_plus>(i);
    BOOST_CHECK_EQUAL(proto::value(proto::child(p2)), 42);

    typedef
        ewrap<
            proto::basic_expr<
                proto::tag::unary_plus
              , proto::list1<
                    ewrap<proto::basic_expr<proto::tag::terminal, proto::term<int> > >
                >
            >
        >
    p3_type;
    p3_type p3 = proto::make_expr<proto::tag::unary_plus, mydomain>(i);
    BOOST_CHECK_EQUAL(proto::value(proto::child(p3)), 42);

    typedef
        ewrap<
            proto::basic_expr<
                proto::tag::plus
              , proto::list2<
                    p3_type
                  , ewrap<proto::basic_expr<proto::tag::terminal, proto::term<int> > >
                >
            >
        >
    p4_type;
    p4_type p4 = proto::make_expr<proto::tag::plus>(p3, 0);
    BOOST_CHECK_EQUAL(proto::value(proto::child(proto::left(p4))), 42);
}

void test_make_expr_ref()
{
    int i = 42;
    proto::terminal<int const &>::type t1 = proto::make_expr<proto::tag::terminal>(boost::cref(1)); // DANGEROUS
    proto::terminal<int &>::type t2 = proto::make_expr<proto::tag::terminal>(boost::ref(i));
    BOOST_CHECK_EQUAL(&i, &proto::value(t2));
    proto::unary_plus<proto::terminal<int const &>::type>::type p1 = proto::make_expr<proto::tag::unary_plus>(boost::cref(1)); // DANGEROUS
    proto::unary_plus<proto::terminal<int &>::type>::type p2 = proto::make_expr<proto::tag::unary_plus>(boost::ref(i));
    BOOST_CHECK_EQUAL(proto::value(proto::child(p2)), 42);

    typedef
        ewrap<
            proto::basic_expr<
                proto::tag::unary_plus
              , proto::list1<
                    ewrap<proto::basic_expr<proto::tag::terminal, proto::term<int &> > >
                >
            >
        >
    p3_type;
    p3_type p3 = proto::make_expr<proto::tag::unary_plus, mydomain>(boost::ref(i));
    BOOST_CHECK_EQUAL(proto::value(proto::child(p3)), 42);

    typedef
        ewrap<
            proto::basic_expr<
                proto::tag::plus
              , proto::list2<
                    p3_type &
                  , ewrap<proto::basic_expr<proto::tag::terminal, proto::term<int> > >
                >
            >
        >
    p4_type;
    p4_type p4 = proto::make_expr<proto::tag::plus>(boost::ref(p3), 0);
    BOOST_CHECK_EQUAL(proto::value(proto::child(proto::left(p4))), 42);
}

void test_make_expr_functional()
{
    int i = 42;
    proto::terminal<int>::type t1 = proto::functional::make_expr<proto::tag::terminal>()(1);
    proto::terminal<int>::type t2 = proto::functional::make_expr<proto::tag::terminal>()(i);
    proto::unary_plus<proto::terminal<int>::type>::type p1 = proto::functional::make_expr<proto::tag::unary_plus>()(1);
    proto::unary_plus<proto::terminal<int>::type>::type p2 = proto::functional::make_expr<proto::tag::unary_plus>()(i);
    BOOST_CHECK_EQUAL(proto::value(proto::child(p2)), 42);

    typedef
        ewrap<
            proto::basic_expr<
                proto::tag::unary_plus
              , proto::list1<
                    ewrap<proto::basic_expr<proto::tag::terminal, proto::term<int> > >
                >
            >
        >
    p3_type;
    p3_type p3 = proto::functional::make_expr<proto::tag::unary_plus, mydomain>()(i);
    BOOST_CHECK_EQUAL(proto::value(proto::child(p3)), 42);

    typedef
        ewrap<
            proto::basic_expr<
                proto::tag::plus
              , proto::list2<
                    p3_type
                  , ewrap<proto::basic_expr<proto::tag::terminal, proto::term<int> > >
                >
            >
        >
    p4_type;
    p4_type p4 = proto::functional::make_expr<proto::tag::plus>()(p3, 0);
}

void test_make_expr_functional_ref()
{
    int i = 42;
    proto::terminal<int const &>::type t1 = proto::functional::make_expr<proto::tag::terminal>()(boost::cref(1)); // DANGEROUS
    proto::terminal<int &>::type t2 = proto::functional::make_expr<proto::tag::terminal>()(boost::ref(i));
    BOOST_CHECK_EQUAL(&i, &proto::value(t2));
    proto::unary_plus<proto::terminal<int const &>::type>::type p1 = proto::functional::make_expr<proto::tag::unary_plus>()(boost::cref(1)); // DANGEROUS
    proto::unary_plus<proto::terminal<int &>::type>::type p2 = proto::functional::make_expr<proto::tag::unary_plus>()(boost::ref(i));
    BOOST_CHECK_EQUAL(proto::value(proto::child(p2)), 42);

    typedef
        ewrap<
            proto::basic_expr<
                proto::tag::unary_plus
              , proto::list1<
                    ewrap<proto::basic_expr<proto::tag::terminal, proto::term<int &> > >
                >
            >
        >
    p3_type;
    p3_type p3 = proto::functional::make_expr<proto::tag::unary_plus, mydomain>()(boost::ref(i));
    BOOST_CHECK_EQUAL(proto::value(proto::child(p3)), 42);

    typedef
        ewrap<
            proto::basic_expr<
                proto::tag::plus
              , proto::list2<
                    p3_type &
                  , ewrap<proto::basic_expr<proto::tag::terminal, proto::term<int> > >
                >
            >
        >
    p4_type;
    p4_type p4 = proto::functional::make_expr<proto::tag::plus>()(boost::ref(p3), 0);
    BOOST_CHECK_EQUAL(proto::value(proto::child(proto::left(p4))), 42);
}

void test_unpack_expr()
{
    int i = 42;
    proto::terminal<int>::type t1 = proto::unpack_expr<proto::tag::terminal>(fusion::make_tuple(1));
    proto::terminal<int &>::type t2 = proto::unpack_expr<proto::tag::terminal>(fusion::make_tuple(boost::ref(i)));
    proto::unary_plus<proto::terminal<int>::type>::type p1 = proto::unpack_expr<proto::tag::unary_plus>(fusion::make_tuple(1));
    proto::unary_plus<proto::terminal<int &>::type>::type p2 = proto::unpack_expr<proto::tag::unary_plus>(fusion::make_tuple(boost::ref(i)));
    BOOST_CHECK_EQUAL(proto::value(proto::child(p2)), 42);

    typedef
        ewrap<
            proto::basic_expr<
                proto::tag::unary_plus
              , proto::list1<
                    ewrap<proto::basic_expr<proto::tag::terminal, proto::term<int &> > >
                >
            >
        >
    p3_type;
    p3_type p3 = proto::unpack_expr<proto::tag::unary_plus, mydomain>(fusion::make_tuple(boost::ref(i)));
    BOOST_CHECK_EQUAL(proto::value(proto::child(p3)), 42);

    typedef
        ewrap<
            proto::basic_expr<
                proto::tag::plus
              , proto::list2<
                    p3_type &
                  , ewrap<proto::basic_expr<proto::tag::terminal, proto::term<int> > >
                >
            >
        >
    p4_type;
    p4_type p4 = proto::unpack_expr<proto::tag::plus>(fusion::make_tuple(boost::ref(p3), 0));
    BOOST_CHECK_EQUAL(proto::value(proto::child(proto::left(p4))), 42);
}

void test_unpack_expr_functional()
{
    int i = 42;
    proto::terminal<int>::type t1 = proto::functional::unpack_expr<proto::tag::terminal>()(fusion::make_tuple(1));
    proto::terminal<int &>::type t2 = proto::functional::unpack_expr<proto::tag::terminal>()(fusion::make_tuple(boost::ref(i)));
    proto::unary_plus<proto::terminal<int>::type>::type p1 = proto::functional::unpack_expr<proto::tag::unary_plus>()(fusion::make_tuple(1));
    proto::unary_plus<proto::terminal<int &>::type>::type p2 = proto::functional::unpack_expr<proto::tag::unary_plus>()(fusion::make_tuple(boost::ref(i)));
    BOOST_CHECK_EQUAL(proto::value(proto::child(p2)), 42);

    typedef
        ewrap<
            proto::basic_expr<
                proto::tag::unary_plus
              , proto::list1<
                    ewrap<proto::basic_expr<proto::tag::terminal, proto::term<int &> > >
                >
            >
        >
    p3_type;
    p3_type p3 = proto::functional::unpack_expr<proto::tag::unary_plus, mydomain>()(fusion::make_tuple(boost::ref(i)));
    BOOST_CHECK_EQUAL(proto::value(proto::child(p3)), 42);

    typedef
        ewrap<
            proto::basic_expr<
                proto::tag::plus
              , proto::list2<
                    p3_type &
                  , ewrap<proto::basic_expr<proto::tag::terminal, proto::term<int> > >
                >
            >
        >
    p4_type;
    p4_type p4 = proto::functional::unpack_expr<proto::tag::plus>()(fusion::make_tuple(boost::ref(p3), 0));
    BOOST_CHECK_EQUAL(proto::value(proto::child(proto::left(p4))), 42);
}

#if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
#define _byref(x) call<proto::_byref(x)>
#define _byval(x) call<proto::_byval(x)>
#define Minus(x) proto::call<Minus(x)>
#endif

// Turn all terminals held by reference into ones held by value
struct ByVal
  : proto::or_<
        proto::when<proto::terminal<proto::_>, proto::_make_terminal(proto::_byval(proto::_value))>
      , proto::when<proto::nary_expr<proto::_, proto::vararg<ByVal> > >
    >
{};

// Turn all terminals held by value into ones held by reference (not safe in general)
struct ByRef
  : proto::or_<
        proto::when<proto::terminal<proto::_>, proto::_make_terminal(proto::_byref(proto::_value))>
      , proto::when<proto::nary_expr<proto::_, proto::vararg<ByRef> > >
    >
{};

// turn all proto::plus nodes to minus nodes:
struct Minus
  : proto::or_<
        proto::when<proto::terminal<proto::_> >
      , proto::when<proto::plus<Minus, Minus>, proto::_make_minus(Minus(proto::_left), Minus(proto::_right)) >
    >
{};

struct Square
  : proto::or_<
        // Not creating new proto::terminal nodes here,
        // so hold the existing terminals by reference:
        proto::when<proto::terminal<proto::_>, proto::_make_multiplies(proto::_, proto::_)>
      , proto::when<proto::plus<Square, Square> >
    >
{};

#if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
#undef _byref
#undef _byval
#undef Minus
#endif

void test_make_expr_transform()
{
    proto::plus<
        proto::terminal<int>::type
      , proto::terminal<int>::type
    >::type t1 = ByVal()(proto::as_expr(1) + 1);

    proto::plus<
        proto::terminal<int const &>::type
      , proto::terminal<int const &>::type
    >::type t2 = ByRef()(proto::as_expr(1) + 1);

    proto::minus<
        proto::terminal<int>::type const &
      , proto::terminal<int const &>::type const &
    >::type t3 = Minus()(proto::as_expr(1) + 1);

    proto::plus<
        proto::multiplies<proto::terminal<int>::type const &, proto::terminal<int>::type const &>::type
      , proto::multiplies<proto::terminal<int const &>::type const &, proto::terminal<int const &>::type const &>::type
    >::type t4 = Square()(proto::as_expr(1) + 1);
}


struct length_impl {};
struct dot_impl {};

proto::terminal<length_impl>::type const length = {{}};
proto::terminal<dot_impl>::type const dot = {{}};

// work around msvc bugs...
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1500))
#define _byref(a) call<proto::_byref(a)>
#define _byval(a) call<proto::_byval(a)>
#define _child1(a) call<proto::_child1(a)>
#define _make_terminal(a) call<proto::_make_terminal(a)>
#define _make_function(a,b,c) call<proto::_make_function(a,b,c)>
#define dot_impl() proto::make<dot_impl()>
#endif

// convert length(a) < length(b) to dot(a,a) < dot(b,b)
struct Convert
  : proto::when<
        proto::less<
            proto::function<proto::terminal<length_impl>, proto::_>
          , proto::function<proto::terminal<length_impl>, proto::_>
        >
      , proto::_make_less(
            proto::_make_function(
                proto::_make_terminal(dot_impl())
              , proto::_child1(proto::_child0)
              , proto::_child1(proto::_child0)
            )
          , proto::_make_function(
                proto::_make_terminal(dot_impl())
              , proto::_child1(proto::_child1)
              , proto::_child1(proto::_child1)
            )
        )
    >
{};

template<typename Expr>
void test_make_expr_transform2_test(Expr const &expr)
{
    void const *addr1 = boost::addressof(proto::child_c<1>(proto::child_c<0>(expr)));
    void const *addr2 = boost::addressof(proto::child_c<1>(proto::child_c<0>(Convert()(expr))));
    BOOST_CHECK_EQUAL(addr1, addr2);

    BOOST_CHECK_EQUAL(1, proto::value(proto::child_c<1>(proto::child_c<0>(expr))));
    BOOST_CHECK_EQUAL(1, proto::value(proto::child_c<1>(proto::child_c<0>(Convert()(expr)))));
}

void test_make_expr_transform2()
{
    test_make_expr_transform2_test(length(1) < length(2));
}

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1500))
#undef _byref
#undef _byval
#undef _child1
#undef _make_terminal
#undef _make_function
#undef dot_impl
#endif

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test proto::make_expr, proto::unpack_expr and friends");

    test->add(BOOST_TEST_CASE(&test_make_expr));
    test->add(BOOST_TEST_CASE(&test_make_expr_ref));
    test->add(BOOST_TEST_CASE(&test_make_expr_functional));
    test->add(BOOST_TEST_CASE(&test_make_expr_functional_ref));
    test->add(BOOST_TEST_CASE(&test_unpack_expr));
    test->add(BOOST_TEST_CASE(&test_unpack_expr_functional));
    test->add(BOOST_TEST_CASE(&test_make_expr_transform));
    test->add(BOOST_TEST_CASE(&test_make_expr_transform2));

    return test;
}
