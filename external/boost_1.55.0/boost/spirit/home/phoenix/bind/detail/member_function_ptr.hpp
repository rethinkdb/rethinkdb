/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_PP_IS_ITERATING)
#if !defined(PHOENIX_BIND_DETAIL_MEMBER_FUNCTION_PTR_HPP)
#define PHOENIX_BIND_DETAIL_MEMBER_FUNCTION_PTR_HPP

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/dec.hpp>
#include <boost/utility/addressof.hpp>

namespace boost { namespace phoenix { namespace detail
{
    template <int N>
    struct member_function_ptr_impl
    {
        template <typename RT, typename FP>
        struct impl;
    };

    template <int N, typename RT, typename FP>
    struct member_function_ptr
        : member_function_ptr_impl<N>::template impl<RT, FP>
    {
        typedef typename member_function_ptr_impl<N>::
            template impl<RT, FP> base;
        member_function_ptr(FP fp)
            : base(fp) {}
    };

    template <>
    struct member_function_ptr_impl<0>
    {
        template <typename RT, typename FP>
        struct impl
        {
            template <typename Class>
            struct result
            {
                typedef RT type;
            };

            impl(FP fp)
                : fp(fp) {}

            template <typename Class>
            RT operator()(Class& obj) const
            {
                return (obj.*fp)();
            }

            template <typename Class>
            RT operator()(Class* obj) const
            {
                return (obj->*fp)();
            }

            FP fp;
        };
    };

#define BOOST_PP_ITERATION_PARAMS_1                                             \
    (3, (1, PHOENIX_COMPOSITE_LIMIT,                                            \
    "boost/spirit/home/phoenix/bind/detail/member_function_ptr.hpp"))
#include BOOST_PP_ITERATE()

}}} // namespace boost::phoenix::detail

#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Preprocessor vertical repetition code
//
///////////////////////////////////////////////////////////////////////////////
#else // defined(BOOST_PP_IS_ITERATING)

#define N BOOST_PP_ITERATION()

    template <>
    struct member_function_ptr_impl<N>
    {
        template <typename RT, typename FP>
        struct impl
        {
            template <typename Class, BOOST_PP_ENUM_PARAMS(N, typename T)>
            struct result
            {
                typedef RT type;
            };

            impl(FP fp)
                : fp(fp) {}

            template <typename Class, BOOST_PP_ENUM_PARAMS(N, typename A)>
            RT operator()(Class& obj, BOOST_PP_ENUM_BINARY_PARAMS(N, A, & _)) const
            {
                return (obj.*fp)(BOOST_PP_ENUM_PARAMS(N, _));
            }

            template <typename Class, BOOST_PP_ENUM_PARAMS(N, typename A)>
            RT operator()(Class* obj, BOOST_PP_ENUM_BINARY_PARAMS(N, A, & _)) const
            {
                return (obj->*fp)(BOOST_PP_ENUM_PARAMS(N, _));
            }

            FP fp;
        };
    };

#undef N
#endif // defined(BOOST_PP_IS_ITERATING)


