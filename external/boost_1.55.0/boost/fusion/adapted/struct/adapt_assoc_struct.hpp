/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman
    Copyright (c) 2007 Dan Marsden
    Copyright (c) 2009-2011 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_STRUCT_ADAPT_ASSOC_STRUCT_HPP
#define BOOST_FUSION_ADAPTED_STRUCT_ADAPT_ASSOC_STRUCT_HPP

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/empty.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
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
#include <boost/fusion/adapted/struct/detail/deref_data_impl.hpp>
#include <boost/fusion/adapted/struct/detail/key_of_impl.hpp>
#include <boost/fusion/adapted/struct/detail/value_of_data_impl.hpp>

#define BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_0(X, Y, Z)                       \
    ((X, Y, Z)) BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_1
#define BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_1(X, Y, Z)                       \
    ((X, Y, Z)) BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_0
#define BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_0_END
#define BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_1_END

#define BOOST_FUSION_ADAPT_ASSOC_STRUCT_C_BASE(                                 \
    TEMPLATE_PARAMS_SEQ,NAME_SEQ,I,PREFIX,ATTRIBUTE)                            \
                                                                                \
    BOOST_FUSION_ADAPT_STRUCT_C_BASE(                                           \
        TEMPLATE_PARAMS_SEQ, NAME_SEQ, I, PREFIX, ATTRIBUTE, 3)                 \
                                                                                \
    template<                                                                   \
        BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS(TEMPLATE_PARAMS_SEQ)   \
    >                                                                           \
    struct struct_assoc_key<BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ), I> \
    {                                                                           \
        typedef BOOST_PP_TUPLE_ELEM(3, 2, ATTRIBUTE) type;                      \
    };

#define BOOST_FUSION_ADAPT_ASSOC_STRUCT_C(                                      \
    TEMPLATE_PARAMS_SEQ,NAME_SEQ, I, ATTRIBUTE)                                 \
                                                                                \
    BOOST_FUSION_ADAPT_ASSOC_STRUCT_C_BASE(                                     \
        TEMPLATE_PARAMS_SEQ,NAME_SEQ,I,BOOST_PP_EMPTY,ATTRIBUTE)

#define BOOST_FUSION_ADAPT_ASSOC_TPL_STRUCT(                                    \
    TEMPLATE_PARAMS_SEQ, NAME_SEQ, ATTRIBUTES)                                  \
                                                                                \
    BOOST_FUSION_ADAPT_STRUCT_BASE(                                             \
        (1)TEMPLATE_PARAMS_SEQ,                                                 \
        (1)NAME_SEQ,                                                            \
        assoc_struct_tag,                                                       \
        0,                                                                      \
        BOOST_PP_CAT(                                                           \
            BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_0(0,0,0)ATTRIBUTES,_END),    \
        BOOST_FUSION_ADAPT_ASSOC_STRUCT_C)

#define BOOST_FUSION_ADAPT_ASSOC_STRUCT(NAME, ATTRIBUTES)                       \
    BOOST_FUSION_ADAPT_STRUCT_BASE(                                             \
        (0),                                                                    \
        (0)(NAME),                                                              \
        assoc_struct_tag,                                                       \
        0,                                                                      \
        BOOST_PP_CAT(                                                           \
            BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_0(0,0,0)ATTRIBUTES,_END),    \
        BOOST_FUSION_ADAPT_ASSOC_STRUCT_C)

#define BOOST_FUSION_ADAPT_ASSOC_STRUCT_AS_VIEW(NAME, ATTRIBUTES)               \
    BOOST_FUSION_ADAPT_STRUCT_BASE(                                             \
        (0),                                                                    \
        (0)(NAME),                                                              \
        assoc_struct_tag,                                                       \
        1,                                                                      \
        BOOST_PP_CAT(                                                           \
            BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_0(0,0,0)ATTRIBUTES,_END),    \
        BOOST_FUSION_ADAPT_ASSOC_STRUCT_C)

#endif
