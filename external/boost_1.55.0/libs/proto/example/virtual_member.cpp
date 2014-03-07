//[ VirtualMember
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This example demonstrates how to use BOOST_PROTO_EXTENDS_MEMBERS()
// to add "virtual" data members to expressions within a domain. For
// instance, with Phoenix you can create a lambda expression such as
//
//    if_(_1 > 0)[ std::cout << _2 ].else_[ std::cout << _3 ]
//
// In the above expression, "else_" is a so-called virtual data member
// of the expression "if_(_1 > 0)[ std::cout << _2 ]". This example
// shows how to implement the ".else_" syntax with Proto.
//
// ****WARNING****WARNING****WARNING****WARNING****WARNING****WARNING****
// * The virtual data member feature is experimental and can change at  *
// * any time. Use it at your own risk.                                 *
// **********************************************************************

#if defined(_MSC_VER) && _MSC_VER == 1310
#error "Sorry, this example doesn\'t work with MSVC 7.1"
#endif

#include <iostream>
#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/min_max.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/typeof/std/ostream.hpp>
#include <boost/proto/proto.hpp>

namespace mpl = boost::mpl;
namespace proto = boost::proto;
namespace fusion = boost::fusion;
using proto::_;

namespace mini_lambda
{
    // A callable PolymorphicFunctionObject that wraps
    // fusion::at()
    struct at : proto::callable
    {
        template<class Sig>
        struct result;
        
        template<class This, class Vector, class N>
        struct result<This(Vector, N)>
          : fusion::result_of::at<
                typename boost::remove_reference<Vector>::type
              , typename boost::remove_reference<N>::type
            >
        {};
        
        template<class Vector, class N>
        typename fusion::result_of::at<Vector const, N>::type
        operator()(Vector const &vector, N) const
        {
            return fusion::at<N>(vector);
        }
    };

    // An MPL IntegralConstant
    template<class N>
    struct placeholder
    {
        typedef N type;
        typedef typename N::tag tag;
        typedef typename N::next next;
        typedef typename N::prior prior;
        typedef typename N::value_type value_type;
        static const value_type value = N::value;
    };

    // Some keyword types for our lambda EDSL
    namespace keyword
    {
        struct if_ {};
        struct else_ {};
        struct do_ {};
        struct while_ {};
        struct try_ {};
        struct catch_ {};
    }

    // Forward declaration for the mini-lambda grammar
    struct eval_if_else;

    // Forward declaration for the mini-lambda expression wrapper
    template<class E>
    struct expression;

    // The grammar for mini-lambda expressions with transforms for
    // evaluating the lambda expression.
    struct grammar
      : proto::or_<
            // When evaluating a placeholder, use the placeholder
            // to index into the "data" parameter, which is a fusion
            // vector containing the arguments to the lambda expression.
            proto::when<
                proto::terminal<placeholder<_> >
              , at(proto::_data, proto::_value)
            >
            // When evaluating if/then/else expressions of the form
            // "if_( E0 )[ E1 ].else_[ E2 ]", pass E0, E1 and E2 to
            // eval_if_else along with the "data" parameter. Note the
            // use of proto::member<> to match binary expressions like
            // "X.Y" where "Y" is a virtual data member.
          , proto::when<
                proto::subscript<
                    proto::member<
                        proto::subscript<
                            proto::function<
                                proto::terminal<keyword::if_>
                              , grammar
                            >
                          , grammar
                        >
                      , proto::terminal<keyword::else_>
                    >
                  , grammar
                >
              , eval_if_else(
                    proto::_right(proto::_left(proto::_left(proto::_left)))
                  , proto::_right(proto::_left(proto::_left))
                  , proto::_right
                  , proto::_data
                )
            >
          , proto::otherwise<
                proto::_default<grammar>
            >
        >
    {};

    // A callable PolymorphicFunctionObject that evaluates
    // if/then/else expressions.
    struct eval_if_else : proto::callable
    {
        typedef void result_type;

        template<typename If, typename Then, typename Else, typename Args>
        void operator()(If const &if_, Then const &then_, Else const &else_, Args const &args) const
        {
            if(grammar()(if_, 0, args))
            {
                grammar()(then_, 0, args);
            }
            else
            {
                grammar()(else_, 0, args);
            }
        }
    };

    // Define the mini-lambda domain, in which all expressions are
    // wrapped in mini_lambda::expression.
    struct domain
      : proto::domain<proto::pod_generator<expression> >
    {};

    // A simple transform for computing the arity of
    // a lambda expression.
    struct arity_of
      : proto::or_<
            proto::when<
                proto::terminal< placeholder<_> >
              , mpl::next<proto::_value>()
            >
          , proto::when<
                proto::terminal<_>
              , mpl::int_<0>()
            >
          , proto::otherwise<
                proto::fold<
                    _
                  , mpl::int_<0>()
                  , mpl::max<arity_of, proto::_state>()
                >
            >
        >
    {};

    // Here is the mini-lambda expression wrapper. It serves two purposes:
    // 1) To define operator() overloads that evaluate the lambda expression, and
    // 2) To define virtual data members like "else_" so that we can write
    //    expressions like "if_(X)[Y].else_[Z]".
    template<class E>
    struct expression
    {
        BOOST_PROTO_BASIC_EXTENDS(E, expression<E>, domain)
        BOOST_PROTO_EXTENDS_ASSIGN()
        BOOST_PROTO_EXTENDS_SUBSCRIPT()

        // Use BOOST_PROTO_EXTENDS_MEMBERS() to define "virtual"
        // data members that all expressions in the mini-lambda
        // domain will have. They can be used to create expressions
        // like "if_(x)[y].else_[z]" and "do_[y].while_(z)".
        BOOST_PROTO_EXTENDS_MEMBERS(
            ((keyword::else_,   else_))
            ((keyword::while_,  while_))
            ((keyword::catch_,  catch_))
        )

        // Calculate the arity of this lambda expression
        static int const arity = boost::result_of<arity_of(E)>::type::value;

        // Define overloads of operator() that evaluate the lambda
        // expression for up to 3 arguments.

        // Don't try to compute the return type of the lambda if
        // it isn't nullary.
        typename mpl::eval_if_c<
            0 != arity
          , mpl::identity<void>
          , boost::result_of<grammar(
                E const &
              , int const &
              , fusion::vector<> &
            )>
        >::type
        operator()() const
        {
            BOOST_MPL_ASSERT_RELATION(arity, ==, 0);
            fusion::vector<> args;
            return grammar()(proto_base(), 0, args);            
        }

        #define BOOST_PROTO_LOCAL_MACRO(                    \
            N, typename_A, A_const_ref, A_const_ref_a, a    \
        )                                                   \
        template<typename_A(N)>                             \
        typename boost::result_of<grammar(                  \
            E const &                                       \
          , int const &                                     \
          , fusion::vector<A_const_ref(N)> &                \
        )>::type                                            \
        operator ()(A_const_ref_a(N)) const                 \
        {                                                   \
            BOOST_MPL_ASSERT_RELATION(arity, <=, N);        \
            fusion::vector<A_const_ref(N)> args(a(N));      \
            return grammar()(proto_base(), 0, args);        \
        }
        // Repeats BOOST_PROTO_LOCAL_MACRO macro for N=1 to 3
        // inclusive (because there are only 3 placeholders)
        #define BOOST_PROTO_LOCAL_a       BOOST_PROTO_a
        #define BOOST_PROTO_LOCAL_LIMITS  (1, 3)
        #include BOOST_PROTO_LOCAL_ITERATE()
    };

    namespace placeholders
    {
        typedef placeholder<mpl::int_<0> > _1_t;
        typedef placeholder<mpl::int_<1> > _2_t;
        typedef placeholder<mpl::int_<2> > _3_t;

        // Define some placeholders
        expression<proto::terminal<_1_t>::type> const _1 = {{{}}};
        expression<proto::terminal<_2_t>::type> const _2 = {{{}}};
        expression<proto::terminal<_3_t>::type> const _3 = {{{}}};

        // Define the if_() statement
        template<typename E>
        typename proto::result_of::make_expr<proto::tag::function, domain
          , keyword::if_
          , E const &
        >::type const
        if_(E const &e)
        {
            return proto::make_expr<proto::tag::function, domain>(
                keyword::if_()
              , boost::ref(e)
            );
        }
    }

    using placeholders::if_;
}

int main()
{
    using namespace mini_lambda::placeholders;

    // OK, we can create if/then/else lambda expressions
    // and evaluate them.    
    if_(_1 > 0)
    [
        std::cout << _2 << '\n'
    ]
    .else_
    [
        std::cout << _3 << '\n'
    ] 
    (-42, "positive", "non-positive");

    // Even though all expressions in the mini-lambda
    // domain have members named else_, while_, and catch_,
    // they all occupy the same byte in the expression.
    BOOST_MPL_ASSERT_RELATION(sizeof(_1), ==, 2);

    return 0;
}
//]
