/*=============================================================================
    Copyright (c) 2001-2009 Joel de Guzman
    Copyright (c) 2009-2010 Hartmut Kaiser
    Copyright (c) 2010-2011 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_ADT_ADAPT_ADT_HPP
#define BOOST_FUSION_ADAPTED_ADT_ADAPT_ADT_HPP

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/empty.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/remove_const.hpp>

#include <boost/fusion/adapted/struct/detail/extension.hpp>
#include <boost/fusion/adapted/struct/detail/adapt_base.hpp>
#include <boost/fusion/adapted/struct/detail/at_impl.hpp>
#include <boost/fusion/adapted/struct/detail/is_view_impl.hpp>
#include <boost/fusion/adapted/struct/detail/is_sequence_impl.hpp>
#include <boost/fusion/adapted/struct/detail/value_at_impl.hpp>
#include <boost/fusion/adapted/struct/detail/category_of_impl.hpp>
#include <boost/fusion/adapted/struct/detail/size_impl.hpp>
#include <boost/fusion/adapted/struct/detail/begin_impl.hpp>
#include <boost/fusion/adapted/struct/detail/end_impl.hpp>
#include <boost/fusion/adapted/struct/detail/value_of_impl.hpp>
#include <boost/fusion/adapted/struct/detail/deref_impl.hpp>
#include <boost/fusion/adapted/adt/detail/extension.hpp>
#include <boost/fusion/adapted/adt/detail/adapt_base.hpp>

#define BOOST_FUSION_ADAPT_ADT_FILLER_0(A, B, C, D)\
    ((A, B, C, D)) BOOST_FUSION_ADAPT_ADT_FILLER_1
#define BOOST_FUSION_ADAPT_ADT_FILLER_1(A, B, C, D)\
    ((A, B, C, D)) BOOST_FUSION_ADAPT_ADT_FILLER_0
#define BOOST_FUSION_ADAPT_ADT_FILLER_0_END
#define BOOST_FUSION_ADAPT_ADT_FILLER_1_END

#define BOOST_FUSION_ADAPT_ADT_C(TEMPLATE_PARAMS_SEQ, NAME_SEQ, I, ATTRIBUTE)   \
    BOOST_FUSION_ADAPT_ADT_C_BASE(                                              \
        TEMPLATE_PARAMS_SEQ, NAME_SEQ, I, ATTRIBUTE, 4)

#define BOOST_FUSION_ADAPT_TPL_ADT(TEMPLATE_PARAMS_SEQ, NAME_SEQ , ATTRIBUTES)  \
    BOOST_FUSION_ADAPT_STRUCT_BASE(                                             \
        (1)TEMPLATE_PARAMS_SEQ,                                                 \
        (1)NAME_SEQ,                                                            \
        struct_tag,                                                             \
        0,                                                                      \
        BOOST_PP_CAT(BOOST_FUSION_ADAPT_ADT_FILLER_0(0,0,0,0)ATTRIBUTES,_END),  \
        BOOST_FUSION_ADAPT_ADT_C)

#define BOOST_FUSION_ADAPT_ADT(NAME, ATTRIBUTES)                                \
    BOOST_FUSION_ADAPT_STRUCT_BASE(                                             \
        (0),                                                                    \
        (0)(NAME),                                                              \
        struct_tag,                                                             \
        0,                                                                      \
        BOOST_PP_CAT(BOOST_FUSION_ADAPT_ADT_FILLER_0(0,0,0,0)ATTRIBUTES,_END),  \
        BOOST_FUSION_ADAPT_ADT_C)

#define BOOST_FUSION_ADAPT_ADT_AS_VIEW(NAME, ATTRIBUTES)                        \
    BOOST_FUSION_ADAPT_STRUCT_BASE(                                             \
        (0),                                                                    \
        (0)(NAME),                                                              \
        struct_tag,                                                             \
        1,                                                                      \
        BOOST_PP_CAT(BOOST_FUSION_ADAPT_ADT_FILLER_0(0,0,0,0)ATTRIBUTES,_END),  \
        BOOST_FUSION_ADAPT_ADT_C)

#endif
