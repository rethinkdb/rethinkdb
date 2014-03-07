//[ FutureGroup
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This is an example of using Proto transforms to implement
// Howard Hinnant's future group proposal.

#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/include/joint_view.hpp>
#include <boost/fusion/include/single_view.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/transform.hpp>
namespace mpl = boost::mpl;
namespace proto = boost::proto;
namespace fusion = boost::fusion;
using proto::_;

template<class L,class R>
struct pick_left
{
    BOOST_MPL_ASSERT((boost::is_same<L, R>));
    typedef L type;
};

// Work-arounds for Microsoft Visual C++ 7.1
#if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
#define FutureGroup(x) proto::call<FutureGroup(x)>
#endif

// Define the grammar of future group expression, as well as a
// transform to turn them into a Fusion sequence of the correct
// type.
struct FutureGroup
  : proto::or_<
        // terminals become a single-element Fusion sequence
        proto::when<
            proto::terminal<_>
          , fusion::single_view<proto::_value>(proto::_value)
        >
        // (a && b) becomes a concatenation of the sequence
        // from 'a' and the one from 'b':
      , proto::when<
            proto::logical_and<FutureGroup, FutureGroup>
          , fusion::joint_view<
                boost::add_const<FutureGroup(proto::_left) >
              , boost::add_const<FutureGroup(proto::_right) >
            >(FutureGroup(proto::_left), FutureGroup(proto::_right))
        >
        // (a || b) becomes the sequence for 'a', so long
        // as it is the same as the sequence for 'b'.
      , proto::when<
            proto::logical_or<FutureGroup, FutureGroup>
          , pick_left<
                FutureGroup(proto::_left)
              , FutureGroup(proto::_right)
            >(FutureGroup(proto::_left))
        >
    >
{};

#if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
#undef FutureGroup
#endif

template<class E>
struct future_expr;

struct future_dom
  : proto::domain<proto::generator<future_expr>, FutureGroup>
{};

// Expressions in the future group domain have a .get()
// member function that (ostensibly) blocks for the futures
// to complete and returns the results in an appropriate
// tuple.
template<class E>
struct future_expr
  : proto::extends<E, future_expr<E>, future_dom>
{
    explicit future_expr(E const &e)
      : future_expr::proto_extends(e)
    {}

    typename fusion::result_of::as_vector<
        typename boost::result_of<FutureGroup(E)>::type
    >::type
    get() const
    {
        return fusion::as_vector(FutureGroup()(*this));
    }
};

// The future<> type has an even simpler .get()
// member function.
template<class T>
struct future
  : future_expr<typename proto::terminal<T>::type>
{
    future(T const &t = T())
      : future::proto_derived_expr(future::proto_base_expr::make(t))
    {}

    T get() const
    {
        return proto::value(*this);
    }
};

// TEST CASES
struct A {};
struct B {};
struct C {};

int main()
{
    using fusion::vector;
    future<A> a;
    future<B> b;
    future<C> c;
    future<vector<A,B> > ab;

    // Verify that various future groups have the
    // correct return types.
    A                       t0 = a.get();
    vector<A, B, C>         t1 = (a && b && c).get();
    vector<A, C>            t2 = ((a || a) && c).get();
    vector<A, B, C>         t3 = ((a && b || a && b) && c).get();
    vector<vector<A, B>, C> t4 = ((ab || ab) && c).get();

    return 0;
}
//]
