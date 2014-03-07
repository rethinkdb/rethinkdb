///////////////////////////////////////////////////////////////////////////////
// lambda.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <boost/mpl/int.hpp>
#include <boost/mpl/min_max.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/fusion/tuple.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/typeof/std/sstream.hpp>
#include <boost/typeof/std/ostream.hpp>
#include <boost/typeof/std/iostream.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
#include <boost/proto/transform.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

using namespace boost;

// Forward declaration of the lambda expression wrapper
template<typename T>
struct lambda;

struct lambda_domain
  : proto::domain<proto::pod_generator<lambda> >
{};

template<typename I>
struct placeholder
{
    typedef I arity;
};

template<typename T>
struct placeholder_arity
{
    typedef typename T::arity type;
};

namespace grammar
{
    using namespace proto;

    // The lambda grammar, with the transforms for calculating the max arity
    struct Lambda
      : or_<
            when< terminal< placeholder<_> >,  mpl::next<placeholder_arity<_value> >() >
          , when< terminal<_>,                 mpl::int_<0>() >
          , when< nary_expr<_, vararg<_> >,    fold<_, mpl::int_<0>(), mpl::max<Lambda,_state>()> >
        >
    {};
}

// simple wrapper for calculating a lambda expression's arity.
template<typename Expr>
struct lambda_arity
  : boost::result_of<grammar::Lambda(Expr, mpl::void_, mpl::void_)>
{};

// The lambda context is the same as the default context
// with the addition of special handling for lambda placeholders
template<typename Tuple>
struct lambda_context
  : proto::callable_context<lambda_context<Tuple> const>
{
    lambda_context(Tuple const &args)
      : args_(args)
    {}

    template<typename Sig>
    struct result;

    template<typename This, typename I>
    struct result<This(proto::tag::terminal, placeholder<I> const &)>
      : fusion::result_of::at<Tuple, I>
    {};

    template<typename I>
    typename fusion::result_of::at<Tuple, I>::type
    operator ()(proto::tag::terminal, placeholder<I> const &) const
    {
        return fusion::at<I>(this->args_);
    }

    Tuple args_;
};

// The lambda<> expression wrapper makes expressions polymorphic
// function objects
template<typename T>
struct lambda
{
    BOOST_PROTO_BASIC_EXTENDS(T, lambda<T>, lambda_domain)
    BOOST_PROTO_EXTENDS_ASSIGN()
    BOOST_PROTO_EXTENDS_SUBSCRIPT()

    // Careful not to evaluate the return type of the nullary function
    // unless we have a nullary lambda!
    typedef typename mpl::eval_if<
        typename lambda_arity<T>::type
      , mpl::identity<void>
      , proto::result_of::eval<T const, lambda_context<fusion::tuple<> > >
    >::type nullary_type;

    // Define our operator () that evaluates the lambda expression.
    nullary_type operator ()() const
    {
        fusion::tuple<> args;
        lambda_context<fusion::tuple<> > ctx(args);
        return proto::eval(*this, ctx);
    }

    #define M0(N, typename_A, A_const_ref, A_const_ref_a, ref_a)                                    \
    template<typename_A(N)>                                                                         \
    typename proto::result_of::eval<T const, lambda_context<fusion::tuple<A_const_ref(N)> > >::type \
    operator ()(A_const_ref_a(N)) const                                                             \
    {                                                                                               \
        fusion::tuple<A_const_ref(N)> args(ref_a(N));                                               \
        lambda_context<fusion::tuple<A_const_ref(N)> > ctx(args);                                   \
        return proto::eval(*this, ctx);                                                             \
    }                                                                                               \
    /**/
    BOOST_PROTO_REPEAT_FROM_TO(1, 4, M0)
    #undef M0
};

// Define some lambda placeholders
lambda<proto::terminal<placeholder<mpl::int_<0> > >::type> const _1 = {{}};
lambda<proto::terminal<placeholder<mpl::int_<1> > >::type> const _2 = {{}};
lambda<proto::terminal<placeholder<mpl::int_<3> > >::type> const _3 = {{}};

template<typename T>
lambda<typename proto::terminal<T>::type> const val(T const &t)
{
    lambda<typename proto::terminal<T>::type> that = {{t}};
    return that;
}

template<typename T>
lambda<typename proto::terminal<T &>::type> const var(T &t)
{
    lambda<typename proto::terminal<T &>::type> that = {{t}};
    return that;
}

void test_lambda()
{
    BOOST_CHECK_EQUAL(11, ( (_1 + 2) / 4 )(42));
    BOOST_CHECK_EQUAL(-11, ( (-(_1 + 2)) / 4 )(42));
    BOOST_CHECK_CLOSE(2.58, ( (4 - _2) * 3 )(42, 3.14), 0.1);

    // check non-const ref terminals
    std::stringstream sout;
    (sout << _1 << " -- " << _2)(42, "Life, the Universe and Everything!");
    BOOST_CHECK_EQUAL("42 -- Life, the Universe and Everything!", sout.str());

    // check nullary lambdas
    BOOST_CHECK_EQUAL(3, (val(1) + val(2))());

    // check array indexing for kicks
    int integers[5] = {0};
    (var(integers)[2] = 2)();
    (var(integers)[_1] = _1)(3);
    BOOST_CHECK_EQUAL(2, integers[2]);
    BOOST_CHECK_EQUAL(3, integers[3]);
}

using namespace unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test expression template domains");

    test->add(BOOST_TEST_CASE(&test_lambda));

    return test;
}
