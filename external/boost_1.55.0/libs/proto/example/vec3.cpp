//[ Vec3
///////////////////////////////////////////////////////////////////////////////
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This is a simple example using proto::extends to extend a terminal type with
// additional behaviors, and using custom contexts and proto::eval for
// evaluating expressions. It is a port of the Vec3 example
// from PETE (http://www.codesourcery.com/pooma/download.html).

#include <iostream>
#include <functional>
#include <boost/assert.hpp>
#include <boost/mpl/int.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
#include <boost/proto/proto_typeof.hpp>
#include <boost/proto/transform.hpp>
namespace mpl = boost::mpl;
namespace proto = boost::proto;
using proto::_;

// Here is an evaluation context that indexes into a Vec3
// expression, and combines the result.
struct Vec3SubscriptCtx
  : proto::callable_context< Vec3SubscriptCtx const >
{
    typedef int result_type;

    Vec3SubscriptCtx(int i)
      : i_(i)
    {}

    // Index array terminals with our subscript. Everything
    // else will be handled by the default evaluation context.
    int operator ()(proto::tag::terminal, int const (&arr)[3]) const
    {
        return arr[this->i_];
    }

    int i_;
};

// Here is an evaluation context that counts the number
// of Vec3 terminals in an expression.
struct CountLeavesCtx
  : proto::callable_context< CountLeavesCtx, proto::null_context >
{
    CountLeavesCtx()
      : count(0)
      {}

      typedef void result_type;

      void operator ()(proto::tag::terminal, int const(&)[3])
      {
          ++this->count;
      }

      int count;
};

struct iplus : std::plus<int>, proto::callable {};

// Here is a transform that does the same thing as the above context.
// It demonstrates the use of the std::plus<> function object
// with the fold transform. With minor modifications, this
// transform could be used to calculate the leaf count at compile
// time, rather than at runtime.
struct CountLeaves
  : proto::or_<
        // match a Vec3 terminal, return 1
        proto::when<proto::terminal<int[3]>, mpl::int_<1>() >
        // match a terminal, return int() (which is 0)
      , proto::when<proto::terminal<_>, int() >
        // fold everything else, using std::plus<> to add
        // the leaf count of each child to the accumulated state.
      , proto::otherwise< proto::fold<_, int(), iplus(CountLeaves, proto::_state) > >
    >
{};

// Here is the Vec3 struct, which is a vector of 3 integers.
struct Vec3
  : proto::extends<proto::terminal<int[3]>::type, Vec3>
{
    explicit Vec3(int i=0, int j=0, int k=0)
    {
        (*this)[0] = i;
        (*this)[1] = j;
        (*this)[2] = k;
    }

    int &operator [](int i)
    {
        return proto::value(*this)[i];
    }

    int const &operator [](int i) const
    {
        return proto::value(*this)[i];
    }

    // Here we define a operator = for Vec3 terminals that
    // takes a Vec3 expression.
    template< typename Expr >
    Vec3 &operator =(Expr const & expr)
    {
        typedef Vec3SubscriptCtx const CVec3SubscriptCtx;
        (*this)[0] = proto::eval(proto::as_expr(expr), CVec3SubscriptCtx(0));
        (*this)[1] = proto::eval(proto::as_expr(expr), CVec3SubscriptCtx(1));
        (*this)[2] = proto::eval(proto::as_expr(expr), CVec3SubscriptCtx(2));
        return *this;
    }

    void print() const
    {
        std::cout << '{' << (*this)[0]
                  << ", " << (*this)[1]
                  << ", " << (*this)[2]
                  << '}' << std::endl;
    }
};

// The count_leaves() function uses the CountLeaves transform and
// to count the number of leaves in an expression.
template<typename Expr>
int count_leaves(Expr const &expr)
{
    // Count the number of Vec3 terminals using the
    // CountLeavesCtx evaluation context.
    CountLeavesCtx ctx;
    proto::eval(expr, ctx);

    // This is another way to count the leaves using a transform.
    int i = 0;
    BOOST_ASSERT( CountLeaves()(expr, i, i) == ctx.count );

    return ctx.count;
}

int main()
{
    Vec3 a, b, c;

    c = 4;

    b[0] = -1;
    b[1] = -2;
    b[2] = -3;

    a = b + c;

    a.print();

    Vec3 d;
    BOOST_PROTO_AUTO(expr1, b + c);
    d = expr1;
    d.print();

    int num = count_leaves(expr1);
    std::cout << num << std::endl;

    BOOST_PROTO_AUTO(expr2, b + 3 * c);
    num = count_leaves(expr2);
    std::cout << num << std::endl;

    BOOST_PROTO_AUTO(expr3, b + c * d);
    num = count_leaves(expr3);
    std::cout << num << std::endl;

    return 0;
}
//]
