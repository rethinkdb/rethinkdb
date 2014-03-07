/*=============================================================================
    Copyright (c) 2011 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !BOOST_PHOENIX_IS_ITERATING
#ifndef BOOST_PHOENIX_CORE_DETAIL_PHX2_RESULT_HPP
#define BOOST_PHOENIX_CORE_DETAIL_PHX2_RESULT_HPP
#include <boost/phoenix/core/limits.hpp>
#include <boost/phoenix/support/iterate.hpp>
#include <boost/mpl/has_xxx.hpp>

namespace boost { namespace phoenix {
    namespace detail
    {
        BOOST_MPL_HAS_XXX_TRAIT_DEF(result_type)

        template <typename Result>
        struct has_phx2_result_impl
        {
            typedef char yes;
            typedef char (&no)[2];

            template <typename A>
            static yes check(typename A::type *);
            
            
            template <typename A>
            static no check(...);

            static bool const value = (sizeof(yes) == sizeof(check<Result>(0)));
            typedef boost::mpl::bool_<value> type;
        };

        template <typename F, BOOST_PHOENIX_typename_A_void(BOOST_PP_DEC(BOOST_PHOENIX_COMPOSITE_LIMIT)), typename Dummy = void>
        struct has_phx2_result
            : mpl::false_
        {};

        template <typename F, BOOST_PHOENIX_typename_A_void(BOOST_PP_DEC(BOOST_PHOENIX_COMPOSITE_LIMIT)), typename Dummy = void>
        struct phx2_result;
#if !defined(BOOST_PHOENIX_DONT_USE_PREPROCESSED_FILES)
#include <boost/phoenix/core/detail/preprocessed/phx2_result.hpp>
#else
#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 2, line: 0, output: "preprocessed/phx2_result_" BOOST_PHOENIX_LIMIT_STR ".hpp")
#endif
/*=============================================================================
    Copyright (c) 2011 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 1)
#endif


        #define BOOST_PHOENIX_ITERATION_PARAMS                                  \
            (3, (1, BOOST_PP_DEC(BOOST_PHOENIX_COMPOSITE_LIMIT),                \
            <boost/phoenix/core/detail/phx2_result.hpp>))
#include BOOST_PHOENIX_ITERATE()

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(output: null)
#endif

#endif
    }
}}

#endif

#else

        template <typename F, BOOST_PHOENIX_typename_A>
        struct has_phx2_result<F, BOOST_PHOENIX_A>
            : mpl::eval_if<
                has_result_type<F>
              , mpl::false_
              , has_phx2_result_impl<typename F::template result<F(BOOST_PHOENIX_A)> >
            >::type
        {};


        template <typename F, BOOST_PHOENIX_typename_A>
        struct phx2_result<F, BOOST_PHOENIX_A>
        {
            typedef typename F::template result<BOOST_PHOENIX_A>::type type;
        };
        
        template <typename F, BOOST_PHOENIX_typename_A>
        struct phx2_result<F, BOOST_PHOENIX_A_ref>
        {
            typedef typename F::template result<BOOST_PHOENIX_A>::type type;
        };
        
        template <typename F, BOOST_PHOENIX_typename_A>
        struct phx2_result<F, BOOST_PHOENIX_A_const_ref>
        {
            typedef typename F::template result<BOOST_PHOENIX_A>::type type;
        };

#endif
