
//  (C) Copyright Edward Diener 2011,2012,2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_TEMPLATE_HPP)
#define BOOST_TTI_DETAIL_TEMPLATE_HPP

#include <boost/config.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/debug/assert.hpp>
#include <boost/preprocessor/facilities/is_empty.hpp>

#define BOOST_TTI_DETAIL_IS_HELPER_BOOST_PP_NIL

#define BOOST_TTI_DETAIL_IS_NIL(param) \
  BOOST_PP_IS_EMPTY \
    ( \
    BOOST_PP_CAT(BOOST_TTI_DETAIL_IS_HELPER_,param) \
    ) \
/**/

#define BOOST_TTI_DETAIL_TRAIT_ASSERT_NOT_NIL(trait,name,params) \
  BOOST_PP_ASSERT_MSG(0, "The parameter must be BOOST_PP_NIL") \
/**/

#define BOOST_TTI_DETAIL_TRAIT_CHECK_IS_NIL(trait,name,params) \
  BOOST_PP_IIF \
    ( \
    BOOST_TTI_DETAIL_IS_NIL(params), \
    BOOST_TTI_DETAIL_TRAIT_HAS_TEMPLATE, \
    BOOST_TTI_DETAIL_TRAIT_ASSERT_NOT_NIL \
    ) \
    (trait,name,params) \
/**/

#define BOOST_TTI_DETAIL_TRAIT_HAS_TEMPLATE(trait,name,params) \
  BOOST_MPL_HAS_XXX_TEMPLATE_NAMED_DEF(BOOST_PP_CAT(trait,_detail_mpl), name, false) \
  template<class BOOST_TTI_DETAIL_TP_T> \
  struct trait : \
    BOOST_PP_CAT(trait,_detail_mpl)<BOOST_TTI_DETAIL_TP_T> \
    { \
    }; \
/**/

#endif // !BOOST_TTI_DETAIL_TEMPLATE_HPP
