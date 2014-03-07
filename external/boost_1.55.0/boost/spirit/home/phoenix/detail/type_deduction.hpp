/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_DETAIL_TYPE_DEDUCTION_HPP
#define PHOENIX_DETAIL_TYPE_DEDUCTION_HPP

/*=============================================================================

    Return Type Deduction
    [JDG Sept. 15, 2003]

    Before C++ adopts the typeof, there is currently no way to deduce the
    result type of an expression such as x + y. This deficiency is a major
    problem with template metaprogramming; for example, when writing
    forwarding functions that attempt to capture the essence of an
    expression inside a function. Consider the std::plus<T>:

        template <typename T>
        struct plus : public binary_function<T, T, T>
        {
            T operator()(T const& x, T const& y) const
            {
                return x + y;
            }
        };

    What's wrong with this? Well, this functor does not accurately capture
    the behavior of the plus operator. 1) It does not handle the case where
    x and y are of different types (e.g. x is short and y is int). 2) It
    assumes that the arguments and return type are the same (i.e. when
    adding a short and an int, the return type ought to be an int). Due to
    these shortcomings, std::plus<T>(x, y) is a poor substitute for x + y.

    The case where x is short and y is int does not really expose the
    problem. We can simply use std::plus<int> and be happy that the
    operands x and y will simply be converted to an int. The problem
    becomes evident when an operand is a user defined type such as bigint.
    Here, the conversion to bigint is simply not acceptable. Even if the
    unnecessary conversion is tolerable, in generic code, it is not always
    possible to choose the right T type that can accomodate both x and y
    operands.

    To truly model the plus operator, what we need is a polymorphic functor
    that can take arbitrary x and y operands. Here's a rough schematic:

        struct plus
        {
            template <typename X, typename Y>
            unspecified-type
            operator()(X const& x, Y const& y) const
            {
                return x + y;
            }
        };

    Now, we can handle the case where X and Y are arbitrary types. We've
    solved the first problem. To solve the second problem, we need some
    form of return type deduction mechanism. If we had the typeof, it would
    be something like:

        template <typename X, typename Y>
        typeof(X() + Y())
        operator()(X const& x, Y const& y) const
        {
            return x + y;
        }

    Without the typeof facility, it is only possible to wrap an expression
    such as x + y in a function or functor if we are given a hint that
    tells us what the actual result type of such an expression is. Such a
    hint can be in the form of a metaprogram, that, given the types of the
    arguments, will return the result type. Example:

        template <typename X, typename Y>
        struct result_of_plus
        {
            typedef unspecified-type type;
        };

    Given a result_of_plus metaprogram, we can complete our polymorphic
    plus functor:

        struct plus
        {
            template <typename X, typename Y>
            typename result_of_plus<X, Y>::type
            operator()(X const& x, Y const& y) const
            {
                return x + y;
            }
        };

    The process is not automatic. We have to specialize the metaprogram for
    specific argument types. Examples:

        template <>
        struct result_of_plus<short, int>
        {
            typedef int type;
        };

        template <typename T>
        struct result_of_plus<std::complex<T>, std::complex<T> >
        {
            typedef std::complex<T> type;
        };

    To make it easier for the user, specializations are provided for common
    types such as primitive c++ types (e.g. int, char, double, etc.), and
    standard types (e.g. std::complex, iostream, std containers and
    iterators).

    To further improve the ease of use, for user defined classes, we can
    supply a few more basic specializations through metaprogramming using
    heuristics based on canonical operator rules (Such heuristics can be
    found in the LL and Phoenix, for example). For example, it is rather
    common that the result of x += y is X& or the result of x || y is a
    bool. The client is out of luck if her classes do not follow the
    canonical rules. She'll then have to supply her own specialization.

    The type deduction mechanism demostrated below approaches the problem
    not through specialization and heuristics, but through a limited form
    of typeof mechanism. The code does not use heuristics, hence, no
    guessing games. The code takes advantage of the fact that, in general,
    the result type of an expression is related to one its arguments' type.
    For example, x + y, where x has type int and y has type double, has the
    result type double (the second operand type). Another example, x[y]
    where x is a vector<T> and y is a std::size_t, has the result type
    vector<T>::reference (the vector<T>'s reference type type).

    The limited form of type deduction presented can detect common
    relations if the result of a binary or unary operation, given arguments
    x and y with types X and Y (respectively), is X, Y, X&, Y&, X*, Y*, X
    const*, Y const*, bool, int, unsigned, double, container and iterator
    elements (e.g the T, where X is: T[N], T*, vector<T>, map<T>,
    vector<T>::iterator). More arguments/return type relationships can be
    established if needed.

    A set of overloaded test(T) functions capture these argument related
    types. Each test(T) function returns a distinct type that can be used
    to determine the exact type of an expression.

    Consider:

        template <typename X, typename Y>
        x_value_type
        test(X const&);

        template <typename X, typename Y>
        y_value_type
        test(Y const&);

    Given an expression x + y, where x is int and y is double, the call to:

        test<int, double>(x + y)

    will return a y_value_type.

    Now, if we rig x_value_type and y_value_type such that both have unique
    sizes, we can use sizeof(test<X, Y>(x + y)) to determine if the result
    type is either X or Y.

    For example, if:

        sizeof(test<X, Y>(x + y)) == sizeof(y_value_type)

    then, we know for sure that the result of x + y has type Y.

    The same basic scheme can be used to detect more argument-dependent
    return types where the sizeof the test(T) return type is used to index
    through a boost::mpl vector which holds each of the corresponding
    result types.

==============================================================================*/
#include <boost/mpl/vector/vector20.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/static_assert.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/spirit/home/phoenix/detail/local_reference.hpp>

namespace boost
{
    struct error_cant_deduce_type {};
}

namespace boost { namespace type_deduction_detail
{
    typedef char(&bool_value_type)[1];
    typedef char(&int_value_type)[2];
    typedef char(&uint_value_type)[3];
    typedef char(&double_value_type)[4];

    typedef char(&bool_reference_type)[5];
    typedef char(&int_reference_type)[6];
    typedef char(&uint_reference_type)[7];
    typedef char(&double_reference_type)[8];

    typedef char(&x_value_type)[9];
    typedef char(&x_reference_type)[10];
    typedef char(&x_const_pointer_type)[11];
    typedef char(&x_pointer_type)[12];

    typedef char(&y_value_type)[13];
    typedef char(&y_reference_type)[14];
    typedef char(&y_const_pointer_type)[15];
    typedef char(&y_pointer_type)[16];

    typedef char(&container_reference_type)[17];
    typedef char(&container_const_reference_type)[18];
    typedef char(&container_mapped_type)[19];

    typedef char(&cant_deduce_type)[20];

    template <typename T, typename Plain = typename remove_cv<T>::type>
    struct is_basic
        : mpl::or_<
            is_same<Plain, bool>
          , is_same<Plain, int>
          , is_same<Plain, unsigned>
          , is_same<Plain, double>
        > {};

    template <typename C>
    struct reference_type
    {
        typedef typename C::reference type;
    };

    template <typename T>
    struct reference_type<T const>
        : reference_type<T> {};

    template <typename T, std::size_t N>
    struct reference_type<T[N]>
    {
        typedef T& type;
    };

    template <typename T>
    struct reference_type<T*>
    {
        typedef T& type;
    };

    template <typename T>
    struct reference_type<T* const>
    {
        typedef T const& type;
    };

    template <typename C>
    struct const_reference_type
    {
        typedef typename C::const_reference type;
    };

    template <typename C>
    struct mapped_type
    {
        typedef typename C::mapped_type type;
    };

    struct asymmetric;

    template <typename X, typename Y>
    cant_deduce_type
    test(...); // The black hole !!!

    template <typename X, typename Y>
    bool_value_type
    test(bool const&);

    template <typename X, typename Y>
    int_value_type
    test(int const&);

    template <typename X, typename Y>
    uint_value_type
    test(unsigned const&);

    template <typename X, typename Y>
    double_value_type
    test(double const&);

    template <typename X, typename Y>
    bool_reference_type
    test(bool&);

    template <typename X, typename Y>
    int_reference_type
    test(int&);

    template <typename X, typename Y>
    uint_reference_type
    test(unsigned&);

    template <typename X, typename Y>
    double_reference_type
    test(double&);

    template <typename X, typename Y>
    typename disable_if<
        mpl::or_<is_basic<X>, is_const<X> >
      , x_value_type
    >::type
    test(X const&);

    template <typename X, typename Y>
    typename disable_if<
        is_basic<X>
      , x_reference_type
    >::type
    test(X&);

    template <typename X, typename Y>
    typename disable_if<
        mpl::or_<
            is_basic<X>
          , is_const<X>
        >
      , x_const_pointer_type
    >::type
    test(X const*);

    template <typename X, typename Y>
    x_pointer_type
    test(X*);

    template <typename X, typename Y>
    typename disable_if<
        mpl::or_<
            is_basic<Y>
          , is_same<Y, asymmetric>
          , is_const<Y>
          , is_same<X, Y>
        >
      , y_value_type
    >::type
    test(Y const&);

    template <typename X, typename Y>
    typename disable_if<
        mpl::or_<
            is_basic<Y>
          , is_same<Y, asymmetric>
          , is_same<X, Y>
        >
      , y_reference_type
    >::type
    test(Y&);

    template <typename X, typename Y>
    typename disable_if<
        mpl::or_<
            is_same<Y, asymmetric>
          , is_const<Y>
          , is_same<X, Y>
        >
      , y_const_pointer_type
    >::type
    test(Y const*);

    template <typename X, typename Y>
    typename disable_if<
        mpl::or_<
            is_same<Y, asymmetric>
          , is_same<X, Y>
        >
      , y_pointer_type
    >::type
    test(Y*);

    template <typename X, typename Y>
    typename disable_if<
        mpl::or_<
            is_basic<typename X::value_type>
          , is_same<typename add_reference<X>::type, typename X::reference>
        >
      , container_reference_type
    >::type
    test(typename X::reference);

    template <typename X, typename Y, typename Z>
    typename enable_if<
        mpl::and_<
            mpl::or_<is_array<X>, is_pointer<X> >
          , mpl::not_<is_basic<Z> >
          , mpl::not_<is_same<X, Z> >
        >
      , container_reference_type
    >::type
    test(Z&);

    template <typename X, typename Y>
    typename disable_if<
        mpl::or_<
            is_basic<typename X::value_type>
          , is_same<typename add_reference<X>::type, typename X::const_reference>
        >
      , container_const_reference_type
    >::type
    test(typename X::const_reference);

    template <typename X, typename Y>
    typename disable_if<
        is_basic<typename X::mapped_type>
      , container_mapped_type
    >::type
    test(typename X::mapped_type);

    template <typename X, typename Y>
    struct base_result_of
    {
        typedef typename phoenix::detail::unwrap_local_reference<X>::type x_type_;
        typedef typename phoenix::detail::unwrap_local_reference<Y>::type y_type_;
        typedef typename remove_reference<x_type_>::type x_type;
        typedef typename remove_reference<y_type_>::type y_type;

        typedef mpl::vector20<
            mpl::identity<bool>
          , mpl::identity<int>
          , mpl::identity<unsigned>
          , mpl::identity<double>
          , mpl::identity<bool&>
          , mpl::identity<int&>
          , mpl::identity<unsigned&>
          , mpl::identity<double&>
          , mpl::identity<x_type>
          , mpl::identity<x_type&>
          , mpl::identity<x_type const*>
          , mpl::identity<x_type*>
          , mpl::identity<y_type>
          , mpl::identity<y_type&>
          , mpl::identity<y_type const*>
          , mpl::identity<y_type*>
          , reference_type<x_type>
          , const_reference_type<x_type>
          , mapped_type<x_type>
          , mpl::identity<error_cant_deduce_type>
        >
        types;
    };

}} // namespace boost::type_deduction_detail

#define BOOST_RESULT_OF_COMMON(expr, name, Y, SYMMETRY)                         \
    struct name                                                                 \
    {                                                                           \
        typedef type_deduction_detail::base_result_of<X, Y> base_type;          \
        static typename base_type::x_type x;                                    \
        static typename base_type::y_type y;                                    \
                                                                                \
        BOOST_STATIC_CONSTANT(int,                                              \
            size = sizeof(                                                      \
                type_deduction_detail::test<                                    \
                    typename base_type::x_type                                  \
                  , SYMMETRY                                                    \
                >(expr)                                                         \
            ));                                                                 \
                                                                                \
        BOOST_STATIC_CONSTANT(int, index = (size / sizeof(char)) - 1);          \
                                                                                \
        typedef typename mpl::at_c<                                             \
            typename base_type::types, index>::type id;                         \
        typedef typename id::type type;                                         \
    };

#define BOOST_UNARY_RESULT_OF(expr, name)                                       \
    template <typename X>                                                       \
    BOOST_RESULT_OF_COMMON(expr, name,                                          \
        type_deduction_detail::asymmetric, type_deduction_detail::asymmetric)

#define BOOST_BINARY_RESULT_OF(expr, name)                                      \
    template <typename X, typename Y>                                           \
    BOOST_RESULT_OF_COMMON(expr, name, Y, typename base_type::y_type)

#define BOOST_ASYMMETRIC_BINARY_RESULT_OF(expr, name)                           \
    template <typename X, typename Y>                                           \
    BOOST_RESULT_OF_COMMON(expr, name, Y, type_deduction_detail::asymmetric)

#endif
