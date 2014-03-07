
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef GCC_LAMBDA_HPP_
#define GCC_LAMBDA_HPP_

#include <boost/local_function.hpp>
#include <boost/local_function/detail/preprocessor/void_list.hpp>
#include <boost/local_function/detail/preprocessor/line_counter.hpp>
#include <boost/local_function/detail/preprocessor/keyword/return.hpp>
#include <boost/local_function/detail/preprocessor/keyword/const_bind.hpp>
#include <boost/local_function/detail/preprocessor/keyword/bind.hpp>
#include <boost/preprocessor/list/for_each_i.hpp>
#include <boost/preprocessor/list/fold_left.hpp>
#include <boost/preprocessor/list/append.hpp>
#include <boost/preprocessor/list/enum.hpp>
#include <boost/preprocessor/list/adt.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/eat.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/facilities/expand.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/config.hpp>

// PRIVATE //

#define GCC_LAMBDA_SPLIT_BIND_(elem, binds, params, results) \
    (BOOST_PP_LIST_APPEND(binds, (elem, BOOST_PP_NIL)), params, results)

#define GCC_LAMBDA_SPLIT_PARAM_(elem, binds, params, results) \
    (binds, BOOST_PP_LIST_APPEND(params, (elem, BOOST_PP_NIL)), results)

#define GCC_LAMBDA_SPLIT_RESULT_(elem, binds, params, results) \
    (binds, params, BOOST_PP_LIST_APPEND(results, (elem, BOOT_PP_NIL)))

#define GCC_LAMBDA_SPLIT_DISPATCH_(d, binds_params_results, elem) \
    BOOST_PP_IIF(BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_IS_RETURN_FRONT(elem), \
        GCC_LAMBDA_SPLIT_RESULT_ \
    , BOOST_PP_IIF(BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_IS_BIND_FRONT(elem), \
        GCC_LAMBDA_SPLIT_BIND_ \
    , BOOST_PP_IIF(BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_IS_CONST_BIND_FRONT( \
            elem), \
        GCC_LAMBDA_SPLIT_BIND_ \
    , /* no result, no bind, and no const bind so it's param */ \
        GCC_LAMBDA_SPLIT_PARAM_ \
    )))(elem, BOOST_PP_TUPLE_ELEM(3, 0, binds_params_results), \
            BOOST_PP_TUPLE_ELEM(3, 1, binds_params_results), \
            BOOST_PP_TUPLE_ELEM(3, 2, binds_params_results))

#define GCC_LAMBDA_SPLIT_(list) \
    BOOST_PP_LIST_FOLD_LEFT(GCC_LAMBDA_SPLIT_DISPATCH_, \
            (BOOST_PP_NIL, BOOST_PP_NIL, BOOST_PP_NIL), list)

#define GCC_LAMBDA_REMOVE_CONST_BIND_(r, unused, i, elem) \
    BOOST_PP_COMMA_IF(i) \
    BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_CONST_BIND_REMOVE_FRONT(elem)

#define GCC_LAMBDA_RESULT_TYPE_(results) \
    BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_RETURN_REMOVE_FRONT( \
            BOOST_PP_LIST_FIRST(results))

#ifdef BOOST_NO_CXX11_LAMBDAS
//[gcc_lambda_macro
#   define GCC_LAMBDA_(binds, params, results) \
        ({ /* open statement expression (GCC extension only) */ \
        BOOST_LOCAL_FUNCTION( \
            BOOST_PP_LIST_ENUM(BOOST_PP_LIST_APPEND(binds, \
                BOOST_PP_LIST_APPEND(params, \
                    BOOST_PP_IIF(BOOST_PP_LIST_IS_NIL(results), \
                        (return void, BOOST_PP_NIL) /* default for lambdas */ \
                    , \
                        results \
                    )\
                ) \
            )) \
        )
//]
#else
#   define GCC_LAMBDA_(binds, params, results) \
        /* ignore const binding because not supported by C++11 lambdas */ \
        [ BOOST_PP_LIST_FOR_EACH_I(GCC_LAMBDA_REMOVE_CONST_BIND_, ~, binds) ] \
        ( BOOST_PP_LIST_ENUM(params) ) \
        BOOST_PP_IIF(BOOST_PP_LIST_IS_NIL(results), \
            BOOST_PP_TUPLE_EAT(1) /* void result type (default) */ \
        , \
            -> GCC_LAMBDA_RESULT_TYPE_ \
        )(results)
#endif

#define GCC_LAMBDA_TUPLE_(binds_params_results) \
    GCC_LAMBDA_(BOOST_PP_TUPLE_ELEM(3, 0, binds_params_results), \
            BOOST_PP_TUPLE_ELEM(3, 1, binds_params_results), \
            BOOST_PP_TUPLE_ELEM(3, 2, binds_params_results))

//[gcc_lambda_end_macro
#define GCC_LAMBDA_END_(id) \
    BOOST_LOCAL_FUNCTION_NAME(BOOST_PP_CAT(gcc_lambda_, id)) \
    BOOST_PP_CAT(gcc_lambda_, id); \
    }) /* close statement expression (GCC extension only) */
//]

// PUBLIC //

// Same arguments as for local functions but respect to C++11 lambdas:
// const bind v is =v, bind& v is &v, void if no return specified, no = or &.
#ifdef BOOST_NO_CXX11_VARIADIC_MACROS
#   define GCC_LAMBDA(void_or_seq) \
        GCC_LAMBDA_TUPLE_(GCC_LAMBDA_SPLIT_( \
                BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST(void_or_seq)))
#else
#   define GCC_LAMBDA(...) \
        GCC_LAMBDA_TUPLE_(GCC_LAMBDA_SPLIT_( \
                BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST(__VA_ARGS__)))
#endif

#ifdef BOOST_NO_CXX11_LAMBDAS
#   define GCC_LAMBDA_END \
        GCC_LAMBDA_END_(BOOST_LOCAL_FUNCTION_DETAIL_PP_LINE_COUNTER)
#else
#   define GCC_LAMBDA_END /* nothing */
#endif

#endif // #include guard

