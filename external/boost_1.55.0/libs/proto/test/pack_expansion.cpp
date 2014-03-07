///////////////////////////////////////////////////////////////////////////////
// pack_expansion.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/proto/proto.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/typeof/typeof.hpp>

namespace mpl = boost::mpl;
namespace proto = boost::proto;
using proto::_;

template<typename T> T declval();

struct eval_ : proto::callable
{
    template<typename Sig>
    struct result;

#define UNARY_OP(TAG, OP)                                                       \
    template<typename This, typename Arg>                                       \
    struct result<This(proto::tag::TAG, Arg)>                                   \
    {                                                                           \
        BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested, (OP declval<Arg>()))            \
        typedef typename nested::type type;                                     \
    };                                                                          \
                                                                                \
    template<typename Arg>                                                      \
    typename result<eval_(proto::tag::TAG, Arg)>::type                          \
    operator()(proto::tag::TAG, Arg arg) const                                  \
    {                                                                           \
        return OP arg;                                                          \
    }                                                                           \
    /**/

#define BINARY_OP(TAG, OP)                                                      \
    template<typename This, typename Left, typename Right>                      \
    struct result<This(proto::tag::TAG, Left, Right)>                           \
    {                                                                           \
        BOOST_TYPEOF_NESTED_TYPEDEF_TPL(nested, (declval<Left>() OP declval<Right>())) \
        typedef typename nested::type type;                                     \
    };                                                                          \
                                                                                \
    template<typename Left, typename Right>                                     \
    typename result<eval_(proto::tag::TAG, Left, Right)>::type                  \
    operator()(proto::tag::TAG, Left left, Right right) const                   \
    {                                                                           \
        return left OP right;                                                   \
    }                                                                           \
    /**/

    UNARY_OP(negate, -)
    BINARY_OP(plus, +)
    BINARY_OP(minus, -)
    BINARY_OP(multiplies, *)
    BINARY_OP(divides, /)
    /*... others ...*/
};

struct eval1
  : proto::or_<
        proto::when<proto::terminal<_>, proto::_value>
      , proto::otherwise<eval_(proto::tag_of<_>(), eval1(proto::pack(_))...)>
    >
{};

struct eval2
  : proto::or_<
        proto::when<proto::terminal<_>, proto::_value>
      , proto::otherwise<proto::call<eval_(proto::tag_of<_>(), eval2(proto::pack(_))...)> >
    >
{};

void test_call_pack()
{
    proto::terminal<int>::type i = {42};
    int res = eval1()(i);
    BOOST_CHECK_EQUAL(res, 42);
    res = eval1()(i + 2);
    BOOST_CHECK_EQUAL(res, 44);
    res = eval1()(i * 2);
    BOOST_CHECK_EQUAL(res, 84);
    res = eval1()(i * 2 + 4);
    BOOST_CHECK_EQUAL(res, 88);

    res = eval2()(i + 2);
    BOOST_CHECK_EQUAL(res, 44);
    res = eval2()(i * 2);
    BOOST_CHECK_EQUAL(res, 84);
    res = eval2()(i * 2 + 4);
    BOOST_CHECK_EQUAL(res, 88);
}

struct make_pair
  : proto::when<
        proto::binary_expr<_, proto::terminal<int>, proto::terminal<int> >
      , std::pair<int, int>(proto::_value(proto::pack(_))...)
    >
{};

void test_make_pack()
{
    proto::terminal<int>::type i = {42};
    std::pair<int, int> p = make_pair()(i + 43);
    BOOST_CHECK_EQUAL(p.first, 42);
    BOOST_CHECK_EQUAL(p.second, 43);
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test immediate evaluation of proto parse trees");

    test->add(BOOST_TEST_CASE(&test_call_pack));
    test->add(BOOST_TEST_CASE(&test_make_pack));

    return test;
}
