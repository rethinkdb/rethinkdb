/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c)      2011 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_TERMINAL_EXPRESSION_MARCH_24_2011_1210AM)
#define BOOST_SPIRIT_TERMINAL_EXPRESSION_MARCH_24_2011_1210AM

#ifndef BOOST_SPIRIT_USE_PHOENIX_V3

namespace boost { namespace phoenix { namespace detail
{
    namespace expression
    {
        template <
            typename F, typename A0 = void, typename A1 = void
          , typename A2 = void, typename Dummy = void>
        struct function_eval;

        template <typename F, typename A0>
        struct function_eval<F, A0>
        {
            typedef phoenix::actor<
                typename phoenix::as_composite<
                    phoenix::detail::function_eval<1>, F, A0
                >::type
            > type;

            static type make(F f, A0 const & _0)
            {
                return phoenix::compose<
                    phoenix::detail::function_eval<1> >(f, _0);
            }
        };
        
        template <typename F, typename A0, typename A1>
        struct function_eval<F, A0, A1>
        {
            typedef phoenix::actor<
                typename phoenix::as_composite<
                    phoenix::detail::function_eval<2>, F, A0, A1
                >::type
            > type;

            static type make(F f, A0 const & _0, A1 const & _1)
            {
                return phoenix::compose<
                    phoenix::detail::function_eval<2> >(f, _0, _1);
            }
        };

        template <typename F, typename A0, typename A1, typename A2>
        struct function_eval<F, A0, A1, A2>
        {
            typedef phoenix::actor<
                typename phoenix::as_composite<
                    phoenix::detail::function_eval<3>, F, A0, A1, A2
                >::type
            > type;

            static type make(F f, A0 const & _0, A1 const & _1, A2 const & _2)
            {
                return phoenix::compose<
                    phoenix::detail::function_eval<3> >(f, _0, _1, _2);
            }
        };
    }
}}}

#endif // !BOOST_SPIRIT_USE_PHOENIX_V3

#endif
