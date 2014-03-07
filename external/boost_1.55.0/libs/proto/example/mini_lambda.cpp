//[ Lambda
///////////////////////////////////////////////////////////////////////////////
// Copyright 2008 Eric Niebler. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This example builds a simple but functional lambda library using Proto.

#include <iostream>
#include <algorithm>
#include <boost/mpl/int.hpp>
#include <boost/mpl/min_max.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/fusion/tuple.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/typeof/std/ostream.hpp>
#include <boost/typeof/std/iostream.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
#include <boost/proto/transform.hpp>
namespace mpl = boost::mpl;
namespace proto = boost::proto;
namespace fusion = boost::fusion;
using proto::_;

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

// The lambda grammar, with the transforms for calculating the max arity
struct lambda_arity
  : proto::or_<
        proto::when<
            proto::terminal< placeholder<_> >
          , mpl::next<placeholder_arity<proto::_value> >()
        >
      , proto::when< proto::terminal<_>
          , mpl::int_<0>()
        >
      , proto::when<
            proto::nary_expr<_, proto::vararg<_> >
          , proto::fold<_, mpl::int_<0>(), mpl::max<lambda_arity, proto::_state>()>
        >
    >
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

    // Calculate the arity of this lambda expression
    static int const arity = boost::result_of<lambda_arity(T)>::type::value;

    template<typename Sig>
    struct result;

    // Define nested result<> specializations to calculate the return
    // type of this lambda expression. But be careful not to evaluate
    // the return type of the nullary function unless we have a nullary
    // lambda!
    template<typename This>
    struct result<This()>
      : mpl::eval_if_c<
            0 == arity
          , proto::result_of::eval<T const, lambda_context<fusion::tuple<> > >
          , mpl::identity<void>
        >
    {};

    template<typename This, typename A0>
    struct result<This(A0)>
      : proto::result_of::eval<T const, lambda_context<fusion::tuple<A0> > >
    {};

    template<typename This, typename A0, typename A1>
    struct result<This(A0, A1)>
      : proto::result_of::eval<T const, lambda_context<fusion::tuple<A0, A1> > >
    {};

    // Define our operator () that evaluates the lambda expression.
    typename result<lambda()>::type
    operator ()() const
    {
        fusion::tuple<> args;
        lambda_context<fusion::tuple<> > ctx(args);
        return proto::eval(*this, ctx);
    }

    template<typename A0>
    typename result<lambda(A0 const &)>::type
    operator ()(A0 const &a0) const
    {
        fusion::tuple<A0 const &> args(a0);
        lambda_context<fusion::tuple<A0 const &> > ctx(args);
        return proto::eval(*this, ctx);
    }

    template<typename A0, typename A1>
    typename result<lambda(A0 const &, A1 const &)>::type
    operator ()(A0 const &a0, A1 const &a1) const
    {
        fusion::tuple<A0 const &, A1 const &> args(a0, a1);
        lambda_context<fusion::tuple<A0 const &, A1 const &> > ctx(args);
        return proto::eval(*this, ctx);
    }
};

// Define some lambda placeholders
lambda<proto::terminal<placeholder<mpl::int_<0> > >::type> const _1 = {{}};
lambda<proto::terminal<placeholder<mpl::int_<1> > >::type> const _2 = {{}};

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

template<typename T>
struct construct_helper
{
    typedef T result_type; // for TR1 result_of

    T operator()() const
    { return T(); }

    // Generate BOOST_PROTO_MAX_ARITY overloads of the
    // following function call operator.
#define BOOST_PROTO_LOCAL_MACRO(N, typename_A, A_const_ref, A_const_ref_a, a)\
    template<typename_A(N)>                                       \
    T operator()(A_const_ref_a(N)) const                          \
    { return T(a(N)); }
#define BOOST_PROTO_LOCAL_a BOOST_PROTO_a
#include BOOST_PROTO_LOCAL_ITERATE()
};

// Generate BOOST_PROTO_MAX_ARITY-1 overloads of the
// following construct() function template.
#define M0(N, typename_A, A_const_ref, A_const_ref_a, ref_a)      \
template<typename T, typename_A(N)>                               \
typename proto::result_of::make_expr<                             \
    proto::tag::function                                          \
  , lambda_domain                                                 \
  , construct_helper<T>                                           \
  , A_const_ref(N)                                                \
>::type const                                                     \
construct(A_const_ref_a(N))                                       \
{                                                                 \
    return proto::make_expr<                                      \
        proto::tag::function                                      \
      , lambda_domain                                             \
    >(                                                            \
        construct_helper<T>()                                     \
      , ref_a(N)                                                  \
    );                                                            \
}
BOOST_PROTO_REPEAT_FROM_TO(1, BOOST_PROTO_MAX_ARITY, M0)
#undef M0

struct S
{
    S() {}
    S(int i, char c)
    {
        std::cout << "S(" << i << "," << c << ")\n";
    }
};

int main()
{
    // Create some lambda objects and immediately
    // invoke them by applying their operator():
    int i = ( (_1 + 2) / 4 )(42);
    std::cout << i << std::endl; // prints 11

    int j = ( (-(_1 + 2)) / 4 )(42);
    std::cout << j << std::endl; // prints -11

    double d = ( (4 - _2) * 3 )(42, 3.14);
    std::cout << d << std::endl; // prints 2.58

    // check non-const ref terminals
    (std::cout << _1 << " -- " << _2 << '\n')(42, "Life, the Universe and Everything!");
    // prints "42 -- Life, the Universe and Everything!"

    // "Nullary" lambdas work too
    int k = (val(1) + val(2))();
    std::cout << k << std::endl; // prints 3

    // check array indexing for kicks
    int integers[5] = {0};
    (var(integers)[2] = 2)();
    (var(integers)[_1] = _1)(3);
    std::cout << integers[2] << std::endl; // prints 2
    std::cout << integers[3] << std::endl; // prints 3

    // Now use a lambda with an STL algorithm!
    int rgi[4] = {1,2,3,4};
    char rgc[4] = {'a','b','c','d'};
    S rgs[4];

    std::transform(rgi, rgi+4, rgc, rgs, construct<S>(_1, _2));
    return 0;
}
//]
