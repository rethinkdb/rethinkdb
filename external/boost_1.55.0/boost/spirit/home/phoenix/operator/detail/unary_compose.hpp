/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_OPERATOR_DETAIL_UNARY_COMPOSE_HPP
#define PHOENIX_OPERATOR_DETAIL_UNARY_COMPOSE_HPP

#define PHOENIX_UNARY_COMPOSE(eval_name, op)                                    \
    template <typename T0>                                                      \
    inline actor<typename as_composite<eval_name, actor<T0> >::type>            \
    operator op (actor<T0> const& a0)                                           \
    {                                                                           \
        return compose<eval_name>(a0);                                          \
    }

#endif
