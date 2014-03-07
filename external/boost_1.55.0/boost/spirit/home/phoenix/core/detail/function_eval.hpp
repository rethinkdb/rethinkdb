/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_PP_IS_ITERATING)
#if !defined(PHOENIX_CORE_DETAIL_FUNCTION_EVAL_HPP)
#define PHOENIX_CORE_DETAIL_FUNCTION_EVAL_HPP

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/enum.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/dec.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_reference.hpp>

// we assume that mpl::vectorN, where N = PHOENIX_COMPOSITE_LIMIT
// is included already.

namespace boost { namespace phoenix { namespace detail
{
    template <int N>
    struct function_eval;

    template <>
    struct function_eval<0>
    {
        template <typename Env, typename F>
        struct result
        {
            typedef typename
                remove_reference<
                    typename F::template result<Env>::type 
                >::type
            fn;
            typedef typename fn::result_type type;
        };

        template <typename RT, typename Env, typename F>
        static RT
        eval(Env const& env, F const& f)
        {
            return f.eval(env)();
        }
    };

    template <typename T>
    T& help_rvalue_deduction(T& x)
    {
        return x;
    }

    template <typename T>
    T const& help_rvalue_deduction(T const& x)
    {
        return x;
    }

// When we call f(_0, _1...) we remove the reference when deducing f's
// return type. $$$ Explain why $$$

#define PHOENIX_GET_ARG(z, n, data)                                             \
    typedef typename                                                            \
        remove_reference<                                                       \
            typename BOOST_PP_CAT(A, n)::template result<Env>::type             \
        >::type                                                                 \
    BOOST_PP_CAT(a, n);

#define PHOENIX_EVAL_ARG(z, n, data)                                            \
    help_rvalue_deduction(BOOST_PP_CAT(_, n).eval(env))

#define BOOST_PP_ITERATION_PARAMS_1                                             \
    (3, (1, BOOST_PP_DEC(PHOENIX_COMPOSITE_LIMIT),                              \
    "boost/spirit/home/phoenix/core/detail/function_eval.hpp"))
#include BOOST_PP_ITERATE()

}}} // namespace boost::phoenix::detail

#undef PHOENIX_GET_ARG
#undef PHOENIX_EVAL_ARG
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Preprocessor vertical repetition code
//
///////////////////////////////////////////////////////////////////////////////
#else // defined(BOOST_PP_IS_ITERATING)

#define N BOOST_PP_ITERATION()

    template <>
    struct function_eval<N>
    {
        template <typename Env, typename F
          , BOOST_PP_ENUM_PARAMS(N, typename A)>
        struct result
        {
            typedef typename
                remove_reference<
                    typename F::template result<Env>::type 
                >::type
            fn;
            BOOST_PP_REPEAT(N, PHOENIX_GET_ARG, _)

            typedef BOOST_PP_CAT(mpl::vector, N)
                <BOOST_PP_ENUM_PARAMS(N, a)>
            args;

            typedef typename
                fn::template result<BOOST_PP_ENUM_PARAMS(N, a)>
            function_apply;

            typedef typename
                mpl::eval_if<
                    is_same<
                        typename mpl::find<args, fusion::void_>::type
                      , typename mpl::end<args>::type>
                  , function_apply
                  , mpl::identity<fusion::void_>
                >::type
            type;
        };

        template <typename RT, typename Env, typename F
          , BOOST_PP_ENUM_PARAMS(N, typename A)>
        static RT
        eval(Env const& env, F const& f
          , BOOST_PP_ENUM_BINARY_PARAMS(N, A, & _))
        {
            return f.eval(env)(BOOST_PP_ENUM(N, PHOENIX_EVAL_ARG, _));
        }
    };

#undef N
#endif // defined(BOOST_PP_IS_ITERATING)


