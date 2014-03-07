//[ LazyVector
///////////////////////////////////////////////////////////////////////////////
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This example constructs a mini-library for linear algebra, using
// expression templates to eliminate the need for temporaries when
// adding vectors of numbers.
//
// This example uses a domain with a grammar to prune the set
// of overloaded operators. Only those operators that produce
// valid lazy vector expressions are allowed.

#include <vector>
#include <iostream>
#include <boost/mpl/int.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
namespace mpl = boost::mpl;
namespace proto = boost::proto;
using proto::_;

template<typename Expr>
struct lazy_vector_expr;

// This grammar describes which lazy vector expressions
// are allowed; namely, vector terminals and addition
// and subtraction of lazy vector expressions.
struct LazyVectorGrammar
  : proto::or_<
        proto::terminal< std::vector<_> >
      , proto::plus< LazyVectorGrammar, LazyVectorGrammar >
      , proto::minus< LazyVectorGrammar, LazyVectorGrammar >
    >
{};

// Tell proto that in the lazy_vector_domain, all
// expressions should be wrapped in laxy_vector_expr<>
// and must conform to the lazy vector grammar.
struct lazy_vector_domain
  : proto::domain<proto::generator<lazy_vector_expr>, LazyVectorGrammar>
{};

// Here is an evaluation context that indexes into a lazy vector
// expression, and combines the result.
template<typename Size = std::size_t>
struct lazy_subscript_context
{
    lazy_subscript_context(Size subscript)
      : subscript_(subscript)
    {}

    // Use default_eval for all the operations ...
    template<typename Expr, typename Tag = typename Expr::proto_tag>
    struct eval
      : proto::default_eval<Expr, lazy_subscript_context>
    {};

    // ... except for terminals, which we index with our subscript
    template<typename Expr>
    struct eval<Expr, proto::tag::terminal>
    {
        typedef typename proto::result_of::value<Expr>::type::value_type result_type;

        result_type operator ()( Expr const & expr, lazy_subscript_context & ctx ) const
        {
            return proto::value( expr )[ ctx.subscript_ ];
        }
    };

    Size subscript_;
};

// Here is the domain-specific expression wrapper, which overrides
// operator [] to evaluate the expression using the lazy_subscript_context.
template<typename Expr>
struct lazy_vector_expr
  : proto::extends<Expr, lazy_vector_expr<Expr>, lazy_vector_domain>
{
    lazy_vector_expr( Expr const & expr = Expr() )
      : lazy_vector_expr::proto_extends( expr )
    {}

    // Use the lazy_subscript_context<> to implement subscripting
    // of a lazy vector expression tree.
    template< typename Size >
    typename proto::result_of::eval< Expr, lazy_subscript_context<Size> >::type
    operator []( Size subscript ) const
    {
        lazy_subscript_context<Size> ctx(subscript);
        return proto::eval(*this, ctx);
    }
};

// Here is our lazy_vector terminal, implemented in terms of lazy_vector_expr
template< typename T >
struct lazy_vector
  : lazy_vector_expr< typename proto::terminal< std::vector<T> >::type >
{
    typedef typename proto::terminal< std::vector<T> >::type expr_type;

    lazy_vector( std::size_t size = 0, T const & value = T() )
      : lazy_vector_expr<expr_type>( expr_type::make( std::vector<T>( size, value ) ) )
    {}

    // Here we define a += operator for lazy vector terminals that
    // takes a lazy vector expression and indexes it. expr[i] here
    // uses lazy_subscript_context<> under the covers.
    template< typename Expr >
    lazy_vector &operator += (Expr const & expr)
    {
        std::size_t size = proto::value(*this).size();
        for(std::size_t i = 0; i < size; ++i)
        {
            proto::value(*this)[i] += expr[i];
        }
        return *this;
    }
};

int main()
{
    // lazy_vectors with 4 elements each.
    lazy_vector< double > v1( 4, 1.0 ), v2( 4, 2.0 ), v3( 4, 3.0 );

    // Add two vectors lazily and get the 2nd element.
    double d1 = ( v2 + v3 )[ 2 ];   // Look ma, no temporaries!
    std::cout << d1 << std::endl;

    // Subtract two vectors and add the result to a third vector.
    v1 += v2 - v3;                  // Still no temporaries!
    std::cout << '{' << v1[0] << ',' << v1[1]
              << ',' << v1[2] << ',' << v1[3] << '}' << std::endl;

    // This expression is disallowed because it does not conform
    // to the LazyVectorGrammar
    //(v2 + v3) += v1;

    return 0;
}
//]
