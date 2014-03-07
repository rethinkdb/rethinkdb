///////////////////////////////////////////////////////////////////////////////
// examples.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/config.hpp>
#include <boost/mpl/min_max.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/transform.hpp>
#include <boost/proto/functional/fusion.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/fusion/include/cons.hpp>
#include <boost/fusion/include/tuple.hpp>
#include <boost/fusion/include/pop_front.hpp>
#include <boost/test/unit_test.hpp>

namespace mpl = boost::mpl;
namespace proto = boost::proto;
namespace fusion = boost::fusion;
using proto::_;

template<int I>
struct placeholder
{};

namespace test1
{
//[ CalcGrammar
    // This is the grammar for calculator expressions,
    // to which we will attach transforms for computing
    // the expressions' arity.
    /*<< A Calculator expression is ... >>*/
    struct CalcArity
      : proto::or_<
            /*<< _1, or ... >>*/
            proto::terminal< placeholder<0> >
          /*<< _2, or ... >>*/
          , proto::terminal< placeholder<1> >
          /*<< some other terminal, or ... >>*/
          , proto::terminal< _ >
          /*<< a unary expression where the operand is a calculator expression, or ... >>*/
          , proto::unary_expr< _, CalcArity >
          /*<< a binary expression where the operands are calculator expressions >>*/
          , proto::binary_expr< _, CalcArity, CalcArity >
        >
    {};
//]
}

//[ binary_arity
/*<< The `CalculatorArity` is a transform for calculating
the arity of a calculator expression. It will be define in
terms of `binary_arity`, which is defined in terms of
`CalculatorArity`; hence, the definition is recursive.>>*/
struct CalculatorArity;

// A custom transform that returns the arity of a unary
// calculator expression by finding the arity of the
// child expression.
struct unary_arity
  /*<< Custom transforms should inherit from
  transform<>. In some cases, (e.g., when the transform
  is a template), it is also necessary to specialize
  the proto::is_callable<> trait. >>*/
  : proto::transform<unary_arity>
{
    template<typename Expr, typename State, typename Data>
    /*<< Transforms have a nested `impl<>` that is
    a valid TR1 function object. >>*/
    struct impl
      : proto::transform_impl<Expr, State, Data>
    {
        /*<< Get the child. >>*/
        typedef typename proto::result_of::child<Expr>::type child_expr;

        /*<< Apply `CalculatorArity` to find the arity of the child. >>*/
        typedef typename boost::result_of<CalculatorArity(child_expr, State, Data)>::type result_type;

        /*<< The `unary_arity` transform doesn't have an interesting
        runtime counterpart, so just return a default-constructed object
        of the correct type. >>*/
        result_type operator ()(proto::ignore, proto::ignore, proto::ignore) const
        {
            return result_type();
        }
    };
};

// A custom transform that returns the arity of a binary
// calculator expression by finding the maximum of the
// arities of the mpl::int_<2> child expressions.
struct binary_arity
  /*<< All custom transforms should inherit from
  transform. In some cases, (e.g., when the transform
  is a template), it is also necessary to specialize
  the proto::is_callable<> trait. >>*/
  : proto::transform<binary_arity>
{
    template<typename Expr, typename State, typename Data>
    /*<< Transforms have a nested `impl<>` that is
    a valid TR1 function object. >>*/
    struct impl
      : proto::transform_impl<Expr, State, Data>
    {
        /*<< Get the left and right children. >>*/
        typedef typename proto::result_of::left<Expr>::type left_expr;
        typedef typename proto::result_of::right<Expr>::type right_expr;

        /*<< Apply `CalculatorArity` to find the arity of the left and right children. >>*/
        typedef typename boost::result_of<CalculatorArity(left_expr, State, Data)>::type left_arity;
        typedef typename boost::result_of<CalculatorArity(right_expr, State, Data)>::type right_arity;

        /*<< The return type is the maximum of the children's arities. >>*/
        typedef typename mpl::max<left_arity, right_arity>::type result_type;

        /*<< The `unary_arity` transform doesn't have an interesting
        runtime counterpart, so just return a default-constructed object
        of the correct type. >>*/
        result_type operator ()(proto::ignore, proto::ignore, proto::ignore) const
        {
            return result_type();
        }
    };
};
//]

proto::terminal< placeholder<0> >::type const _1 = {{}};
proto::terminal< placeholder<1> >::type const _2 = {{}};

//[ CalculatorArityGrammar
struct CalculatorArity
  : proto::or_<
        proto::when< proto::terminal< placeholder<0> >,  mpl::int_<1>() >
      , proto::when< proto::terminal< placeholder<1> >,  mpl::int_<2>() >
      , proto::when< proto::terminal<_>,                 mpl::int_<0>() >
      , proto::when< proto::unary_expr<_, _>,            unary_arity >
      , proto::when< proto::binary_expr<_, _, _>,        binary_arity >
    >
{};
//]

//[ CalcArity
struct CalcArity
  : proto::or_<
        proto::when< proto::terminal< placeholder<0> >,
            mpl::int_<1>()
        >
      , proto::when< proto::terminal< placeholder<1> >,
            mpl::int_<2>()
        >
      , proto::when< proto::terminal<_>,
            mpl::int_<0>()
        >
      , proto::when< proto::unary_expr<_, CalcArity>,
            CalcArity(proto::_child)
        >
      , proto::when< proto::binary_expr<_, CalcArity, CalcArity>,
            mpl::max<CalcArity(proto::_left),
                     CalcArity(proto::_right)>()
        >
    >
{};
//]

// BUGBUG find workaround for this
#if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
#define _pop_front(x) call<proto::_pop_front(x)>
#define _value(x)     call<proto::_value(x)>
#endif

//[ AsArgList
// This transform matches function invocations such as foo(1,'a',"b")
// and transforms them into Fusion cons lists of their arguments. In this
// case, the result would be cons(1, cons('a', cons("b", nil()))).
struct ArgsAsList
  : proto::when<
        proto::function<proto::terminal<_>, proto::vararg<proto::terminal<_> > >
      /*<< Use a `reverse_fold<>` transform to iterate over the children
      of this node in reverse order, building a fusion list from back to
      front. >>*/
      , proto::reverse_fold<
            /*<< The first child expression of a `function<>` node is the
            function being invoked. We don't want that in our list, so use
            `pop_front()` to remove it. >>*/
            proto::_pop_front(_)
          /*<< `nil` is the initial state used by the `reverse_fold<>`
          transform. >>*/
          , fusion::nil()
          /*<< Put the rest of the function arguments in a fusion cons
          list. >>*/
          , fusion::cons<proto::_value, proto::_state>(proto::_value, proto::_state)
        >
    >
{};
//]

//[ FoldTreeToList
// This transform matches expressions of the form (_1=1,'a',"b")
// (note the use of the comma operator) and transforms it into a
// Fusion cons list of their arguments. In this case, the result
// would be cons(1, cons('a', cons("b", nil()))).
struct FoldTreeToList
  : proto::or_<
        // This grammar describes what counts as the terminals in expressions
        // of the form (_1=1,'a',"b"), which will be flattened using
        // reverse_fold_tree<> below.
        proto::when< proto::assign<_, proto::terminal<_> >
          , proto::_value(proto::_right)
        >
      , proto::when< proto::terminal<_>
          , proto::_value
        >
      , proto::when<
            proto::comma<FoldTreeToList, FoldTreeToList>
          /*<< Fold all terminals that are separated by commas into a Fusion cons list. >>*/
          , proto::reverse_fold_tree<
                _
              , fusion::nil()
              , fusion::cons<FoldTreeToList, proto::_state>(FoldTreeToList, proto::_state)
            >
        >
    >
{};
//]

//[ Promote
// This transform finds all float terminals in an expression and promotes
// them to doubles.
struct Promote
  : proto::or_<
        /*<< Match a `terminal<float>`, then construct a
        `terminal<double>::type` with the `float`. >>*/
        proto::when<proto::terminal<float>, proto::terminal<double>::type(proto::_value) >
      , proto::when<proto::terminal<_> >
      /*<< `nary_expr<>` has a pass-through transform which
      will transform each child sub-expression using the
      `Promote` transform. >>*/
      , proto::when<proto::nary_expr<_, proto::vararg<Promote> > >
    >
{};
//]

//[ LazyMakePair
struct make_pair_tag {};
proto::terminal<make_pair_tag>::type const make_pair_ = {{}};

// This transform matches lazy function invocations like
// `make_pair_(1, 3.14)` and actually builds a `std::pair<>`
// from the arguments.
struct MakePair
  : proto::when<
        /*<< Match expressions like `make_pair_(1, 3.14)` >>*/
        proto::function<
            proto::terminal<make_pair_tag>
          , proto::terminal<_>
          , proto::terminal<_>
        >
      /*<< Return `std::pair<F,S>(f,s)` where `f` and `s` are the
      first and second arguments to the lazy `make_pair_()` function.
      (This uses `proto::make<>` under the covers to evaluate the
      transform.)>>*/
      , std::pair<
            proto::_value(proto::_child1)
          , proto::_value(proto::_child2)
        >(
            proto::_value(proto::_child1)
          , proto::_value(proto::_child2)
        )
    >
{};
//]

namespace lazy_make_pair2
{
    //[ LazyMakePair2
    struct make_pair_tag {};
    proto::terminal<make_pair_tag>::type const make_pair_ = {{}};

    // Like std::make_pair(), only as a function object.
    /*<<Inheriting from `proto::callable` lets Proto know
    that this is a callable transform, so we can use it
    without having to wrap it in `proto::call<>`.>>*/
    struct make_pair : proto::callable
    {
        template<typename Sig> struct result;

        template<typename This, typename First, typename Second>
        struct result<This(First, Second)>
        {
            typedef
                std::pair<
                    BOOST_PROTO_UNCVREF(First)
                  , BOOST_PROTO_UNCVREF(Second)
                >
            type;
        };

        template<typename First, typename Second>
        std::pair<First, Second>
        operator()(First const &first, Second const &second) const
        {
            return std::make_pair(first, second);
        }
    };

    // This transform matches lazy function invocations like
    // `make_pair_(1, 3.14)` and actually builds a `std::pair<>`
    // from the arguments.
    struct MakePair
      : proto::when<
            /*<< Match expressions like `make_pair_(1, 3.14)` >>*/
            proto::function<
                proto::terminal<make_pair_tag>
              , proto::terminal<_>
              , proto::terminal<_>
            >
          /*<< Return `make_pair()(f,s)` where `f` and `s` are the
          first and second arguments to the lazy `make_pair_()` function.
          (This uses `proto::call<>` under the covers  to evaluate the
          transform.)>>*/
          , make_pair(
                proto::_value(proto::_child1)
              , proto::_value(proto::_child2)
            )
        >
    {};
    //]
}


//[ NegateInt
struct NegateInt
  : proto::when<proto::terminal<int>, proto::negate<_>(_)>
{};
//]

#ifndef BOOST_MSVC
//[ SquareAndPromoteInt
struct SquareAndPromoteInt
  : proto::when<
        proto::terminal<int>
      , proto::_make_multiplies(
            proto::terminal<long>::type(proto::_value)
          , proto::terminal<long>::type(proto::_value)
        )
    >
{};
//]
#endif

namespace lambda_transform
{
    //[LambdaTransform
    template<typename N>
    struct placeholder
    {
        typedef typename N::type type;
        static typename N::value_type const value = N::value;
    };

    // A function object that calls fusion::at()
    struct at : proto::callable
    {
        template<typename Sig>
        struct result;

        template<typename This, typename Cont, typename Index>
        struct result<This(Cont, Index)>
          : fusion::result_of::at<
                typename boost::remove_reference<Cont>::type
              , typename boost::remove_reference<Index>::type
            >
        {};

        template<typename Cont, typename Index>
        typename fusion::result_of::at<Cont, Index>::type
        operator ()(Cont &cont, Index const &) const
        {
            return fusion::at<Index>(cont);
        }
    };

    // A transform that evaluates a lambda expression.
    struct LambdaEval
      : proto::or_<
            /*<<When you match a placeholder ...>>*/
            proto::when<
                proto::terminal<placeholder<_> >
              /*<<... call at() with the data parameter, which
              is a tuple, and the placeholder, which is an MPL
              Integral Constant.>>*/
              , at(proto::_data, proto::_value)
            >
            /*<<Otherwise, use the _default<> transform, which
            gives the operators their usual C++ meanings.>>*/
          , proto::otherwise< proto::_default<LambdaEval> >
        >
    {};

    // Define the lambda placeholders
    proto::terminal<placeholder<mpl::int_<0> > >::type const _1 = {{}};
    proto::terminal<placeholder<mpl::int_<1> > >::type const _2 = {{}};

    void test_lambda()
    {
        // a tuple that contains the values
        // of _1 and _2
        fusion::tuple<int, int> tup(2,3);

        // Use LambdaEval to evaluate a lambda expression
        int j = LambdaEval()( _2 - _1, 0, tup );
        BOOST_CHECK_EQUAL(j, 1);

        // You can mutate leaves in an expression tree
        proto::literal<int> k(42);
        int &l = LambdaEval()( k += 4, 0, tup );
        BOOST_CHECK_EQUAL(k.get(), 46);
        BOOST_CHECK_EQUAL(&l, &k.get());

        // You can mutate the values in the tuple, too.
        LambdaEval()( _1 += 4, 0, tup );
        BOOST_CHECK_EQUAL(6, fusion::at_c<0>(tup));
    }
    //]
}

void test_examples()
{
    //[ CalculatorArityTest
    int i = 0; // not used, dummy state and data parameter

    std::cout << CalculatorArity()( proto::lit(100) * 200, i, i) << '\n';
    std::cout << CalculatorArity()( (_1 - _1) / _1 * 100, i, i) << '\n';
    std::cout << CalculatorArity()( (_2 - _1) / _2 * 100, i, i) << '\n';
    //]

    BOOST_CHECK_EQUAL(0, CalculatorArity()( proto::lit(100) * 200, i, i));
    BOOST_CHECK_EQUAL(1, CalculatorArity()( (_1 - _1) / _1 * 100, i, i));
    BOOST_CHECK_EQUAL(2, CalculatorArity()( (_2 - _1) / _2 * 100, i, i));

    BOOST_CHECK_EQUAL(0, CalcArity()( proto::lit(100) * 200, i, i));
    BOOST_CHECK_EQUAL(1, CalcArity()( (_1 - _1) / _1 * 100, i, i));
    BOOST_CHECK_EQUAL(2, CalcArity()( (_2 - _1) / _2 * 100, i, i));

    using boost::fusion::cons;
    using boost::fusion::nil;
    cons<int, cons<char, cons<std::string> > > args(ArgsAsList()( _1(1, 'a', std::string("b")), i, i ));
    BOOST_CHECK_EQUAL(args.car, 1);
    BOOST_CHECK_EQUAL(args.cdr.car, 'a');
    BOOST_CHECK_EQUAL(args.cdr.cdr.car, std::string("b"));

    cons<int, cons<char, cons<std::string> > > lst(FoldTreeToList()( (_1 = 1, 'a', std::string("b")), i, i ));
    BOOST_CHECK_EQUAL(lst.car, 1);
    BOOST_CHECK_EQUAL(lst.cdr.car, 'a');
    BOOST_CHECK_EQUAL(lst.cdr.cdr.car, std::string("b"));

    proto::plus<
        proto::terminal<double>::type
      , proto::terminal<double>::type
    >::type p = Promote()( proto::lit(1.f) + 2.f, i, i );

    //[ LazyMakePairTest
    int j = 0; // not used, dummy state and data parameter

    std::pair<int, double> p2 = MakePair()( make_pair_(1, 3.14), j, j );

    std::cout << p2.first << std::endl;
    std::cout << p2.second << std::endl;
    //]

    BOOST_CHECK_EQUAL(p2.first, 1);
    BOOST_CHECK_EQUAL(p2.second, 3.14);

    std::pair<int, double> p3 = lazy_make_pair2::MakePair()( lazy_make_pair2::make_pair_(1, 3.14), j, j );

    std::cout << p3.first << std::endl;
    std::cout << p3.second << std::endl;

    BOOST_CHECK_EQUAL(p3.first, 1);
    BOOST_CHECK_EQUAL(p3.second, 3.14);

    NegateInt()(proto::lit(1), i, i);
    #ifndef BOOST_MSVC
    SquareAndPromoteInt()(proto::lit(1), i, i);
    #endif

    lambda_transform::test_lambda();
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test examples from the documentation");

    test->add(BOOST_TEST_CASE(&test_examples));

    return test;
}
