///////////////////////////////////////////////////////////////////////////////
// mpl.hpp
//
//  Copyright 2012 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/proto/proto.hpp>
#include <boost/fusion/mpl.hpp>
#include <boost/mpl/pop_back.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>
#include <boost/test/unit_test.hpp>
namespace mpl = boost::mpl;
namespace proto = boost::proto;
namespace fusion = boost::fusion;
using proto::_;

template<class E>
struct my_expr;

struct my_domain
  : proto::domain<proto::generator<my_expr> >
{};

template<class E>
struct my_expr
  : proto::extends<E, my_expr<E>, my_domain>
{
    my_expr(E const &e = E())
      : proto::extends<E, my_expr<E>, my_domain>(e)
    {}

    typedef fusion::fusion_sequence_tag tag;
};

template<typename T>
void test_impl(T const &)
{
    typedef typename mpl::pop_back<T>::type result_type;
    BOOST_STATIC_ASSERT(
        (boost::is_same<
            result_type
          , my_expr<proto::basic_expr<proto::tag::plus, proto::list1<my_expr<proto::terminal<int>::type>&> > >
        >::value)
    );
}

// Test that we can call mpl algorithms on proto expression types, and get proto expression types back
void test_mpl()
{
    my_expr<proto::terminal<int>::type> i;
    test_impl(i + i);
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test proto mpl integration via fusion");

    test->add(BOOST_TEST_CASE(&test_mpl));

    return test;
}
