/*=============================================================================
    Copyright (c) 2005-2007 Dan Marsden
    Copyright (c) 2005-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PP_IS_ITERATING
#ifndef PHOENIX_OPERATOR_DETAIL_MEM_FUN_PTR_RETURN_HPP
#define PHOENIX_OPERATOR_DETAIL_MEM_FUN_PTR_RETURN_HPP

#include <boost/spirit/home/phoenix/core/limits.hpp>

#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>

namespace boost { namespace phoenix {
namespace detail
{
    template<typename MemFunPtr>
    struct mem_fun_ptr_return;

    template<typename Ret, typename Class>
    struct mem_fun_ptr_return<Ret (Class::*)()>
    {
        typedef Ret type;
    };
    
    template<typename Ret, typename Class>
    struct mem_fun_ptr_return<Ret (Class::*)() const>
    {
        typedef Ret type;
    };

#define BOOST_PP_ITERATION_PARAMS_1                                                           \
        (3, (1, PHOENIX_MEMBER_LIMIT, "boost/spirit/home/phoenix/operator/detail/mem_fun_ptr_return.hpp"))

#include BOOST_PP_ITERATE()

}
}}

#endif

#else

#define PHOENIX_ITERATION BOOST_PP_ITERATION()

    template<typename Ret, typename Class,
             BOOST_PP_ENUM_PARAMS(PHOENIX_ITERATION, typename T)>
    struct mem_fun_ptr_return<Ret (Class::*)(BOOST_PP_ENUM_PARAMS(PHOENIX_ITERATION, T))>
    {
        typedef Ret type;
    };

    template<typename Ret, typename Class,
             BOOST_PP_ENUM_PARAMS(PHOENIX_ITERATION, typename T)>
    struct mem_fun_ptr_return<Ret (Class::*)(BOOST_PP_ENUM_PARAMS(PHOENIX_ITERATION, T)) const>
    {
        typedef Ret type;
    };

#undef PHOENIX_ITERATION

#endif
