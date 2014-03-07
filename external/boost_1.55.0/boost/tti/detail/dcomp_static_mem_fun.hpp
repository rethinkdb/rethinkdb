
//  (C) Copyright Edward Diener 2011,2012,2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_COMP_STATIC_MEM_FUN_HPP)
#define BOOST_TTI_DETAIL_COMP_STATIC_MEM_FUN_HPP

#include <boost/config.hpp>
#include <boost/function_types/is_function.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/type_traits/detail/yes_no_type.hpp>
#include <boost/tti/detail/dnullptr.hpp>

#define BOOST_TTI_DETAIL_TRAIT_HAS_COMP_STATIC_MEMBER_FUNCTION(trait,name) \
  template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_TYPE> \
  struct BOOST_PP_CAT(trait,_detail) \
    { \
    template<BOOST_TTI_DETAIL_TP_TYPE *> \
    struct helper; \
    \
    template<class U> \
    static ::boost::type_traits::yes_type chkt(helper<&U::name> *); \
    \
    template<class U> \
    static ::boost::type_traits::no_type chkt(...); \
    \
    BOOST_STATIC_CONSTANT(bool,value=(boost::function_types::is_function<BOOST_TTI_DETAIL_TP_TYPE>::value) && (sizeof(chkt<BOOST_TTI_DETAIL_TP_T>(BOOST_TTI_DETAIL_NULLPTR))==sizeof(::boost::type_traits::yes_type))); \
    \
    typedef boost::mpl::bool_<value> type; \
    }; \
/**/

#endif // BOOST_TTI_DETAIL_COMP_STATIC_MEM_FUN_HPP
