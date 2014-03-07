/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_OPERATOR_DETAIL_BINARY_COMPOSE_HPP
#define PHOENIX_OPERATOR_DETAIL_BINARY_COMPOSE_HPP

#define PHOENIX_BINARY_COMPOSE(eval_name, op)                                   \
    template <typename T0, typename T1>                                         \
    inline actor<typename as_composite<eval_name, actor<T0>, actor<T1> >::type> \
    operator op (actor<T0> const& a0, actor<T1> const& a1)                      \
    {                                                                           \
        return compose<eval_name>(a0, a1);                                      \
    }                                                                           \
                                                                                \
    template <typename T0, typename T1>                                         \
    inline actor<typename as_composite<eval_name, actor<T0>, T1>::type>         \
    operator op (actor<T0> const& a0, T1 const& a1)                             \
    {                                                                           \
        return compose<eval_name>(a0, a1);                                      \
    }                                                                           \
                                                                                \
    template <typename T0, typename T1>                                         \
    inline actor<typename as_composite<eval_name, T0, actor<T1> >::type>        \
    operator op (T0 const& a0, actor<T1> const& a1)                             \
    {                                                                           \
        return compose<eval_name>(a0, a1);                                      \
    }

#endif
