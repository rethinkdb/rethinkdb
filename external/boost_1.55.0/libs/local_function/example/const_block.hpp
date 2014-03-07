
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef CONST_BLOCK_HPP_
#define CONST_BLOCK_HPP_

#include <boost/local_function.hpp>
#include <boost/local_function/detail/preprocessor/void_list.hpp>
#include <boost/local_function/detail/preprocessor/line_counter.hpp>
#include <boost/preprocessor/list/for_each_i.hpp>
#include <boost/preprocessor/list/adt.hpp>
#include <boost/preprocessor/tuple/eat.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/cat.hpp>

// PRIVATE //

#define CONST_BLOCK_BIND_(r, unused, i, var) \
    BOOST_PP_COMMA_IF(i) const bind& var
    
//[const_block_macro
#define CONST_BLOCK_(variables) \
    void BOOST_LOCAL_FUNCTION( \
        BOOST_PP_IIF(BOOST_PP_LIST_IS_NIL(variables), \
            void BOOST_PP_TUPLE_EAT(3) \
        , \
            BOOST_PP_LIST_FOR_EACH_I \
        )(CONST_BLOCK_BIND_, ~, variables) \
    )
//]

//[const_block_end_macro
#define CONST_BLOCK_END_(id) \
    BOOST_LOCAL_FUNCTION_NAME(BOOST_PP_CAT(const_block_, id)) \
    BOOST_PP_CAT(const_block_, id)(); /* call local function immediately */
//]

// PUBLIC //

// Arguments `void | var1, var2, ... | (var1) (var2) ...`.
#ifdef BOOST_NO_CXX11_VARIADIC_MACROS
#   define CONST_BLOCK(void_or_seq) \
        CONST_BLOCK_(BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST(void_or_seq))
#else
#   define CONST_BLOCK(...) \
        CONST_BLOCK_(BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST(__VA_ARGS__))
#endif

#define CONST_BLOCK_END \
    CONST_BLOCK_END_(BOOST_LOCAL_FUNCTION_DETAIL_PP_LINE_COUNTER)

#endif // #include guard

