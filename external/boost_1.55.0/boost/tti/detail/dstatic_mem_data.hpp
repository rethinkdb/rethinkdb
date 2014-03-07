
//  (C) Copyright Edward Diener 2011,2012,2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_STATIC_MEM_DATA_HPP)
#define BOOST_TTI_DETAIL_STATIC_MEM_DATA_HPP

#include <boost/config.hpp>
#include <boost/function_types/is_function.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/type_traits/detail/yes_no_type.hpp>
#include <boost/tti/detail/dnullptr.hpp>

#if defined(BOOST_MSVC)

#define BOOST_TTI_DETAIL_TRAIT_HAS_STATIC_MEMBER_DATA(trait,name) \
  template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_TYPE> \
  struct BOOST_PP_CAT(trait,_detail_hsd) \
    { \
    template<bool,typename BOOST_TTI_DETAIL_TP_U> \
    struct menable_if; \
    \
    template<typename BOOST_TTI_DETAIL_TP_U> \
    struct menable_if<true,BOOST_TTI_DETAIL_TP_U> \
      { \
      typedef BOOST_TTI_DETAIL_TP_U type; \
      }; \
    \
    template<typename BOOST_TTI_DETAIL_TP_U,typename BOOST_TTI_DETAIL_TP_V> \
    static ::boost::type_traits::yes_type check2(BOOST_TTI_DETAIL_TP_V *); \
    \
    template<typename BOOST_TTI_DETAIL_TP_U,typename BOOST_TTI_DETAIL_TP_V> \
    static ::boost::type_traits::no_type check2(BOOST_TTI_DETAIL_TP_U); \
    \
    template<typename BOOST_TTI_DETAIL_TP_U,typename BOOST_TTI_DETAIL_TP_V> \
    static typename \
      menable_if \
        < \
        sizeof(check2<BOOST_TTI_DETAIL_TP_U,BOOST_TTI_DETAIL_TP_V>(&BOOST_TTI_DETAIL_TP_U::name))==sizeof(::boost::type_traits::yes_type), \
        ::boost::type_traits::yes_type \
        > \
      ::type \
    has_matching_member(int); \
    \
    template<typename BOOST_TTI_DETAIL_TP_U,typename BOOST_TTI_DETAIL_TP_V> \
    static ::boost::type_traits::no_type has_matching_member(...); \
    \
    template<class BOOST_TTI_DETAIL_TP_U,class BOOST_TTI_DETAIL_TP_V> \
    struct ttc_sd \
      { \
      typedef boost::mpl::bool_<sizeof(has_matching_member<BOOST_TTI_DETAIL_TP_V,BOOST_TTI_DETAIL_TP_U>(0))==sizeof(::boost::type_traits::yes_type)> type; \
      }; \
    \
    typedef typename ttc_sd<BOOST_TTI_DETAIL_TP_TYPE,BOOST_TTI_DETAIL_TP_T>::type type; \
    \
    BOOST_STATIC_CONSTANT(bool,value=type::value); \
    }; \
/**/

#else // !defined(BOOST_MSVC)

#define BOOST_TTI_DETAIL_TRAIT_HAS_STATIC_MEMBER_DATA(trait,name) \
  template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_TYPE> \
  struct BOOST_PP_CAT(trait,_detail_hsd) \
    { \
    template<BOOST_TTI_DETAIL_TP_TYPE *> \
    struct helper; \
    \
    template<class BOOST_TTI_DETAIL_TP_U> \
    static ::boost::type_traits::yes_type chkt(helper<&BOOST_TTI_DETAIL_TP_U::name> *); \
    \
    template<class BOOST_TTI_DETAIL_TP_U> \
    static ::boost::type_traits::no_type chkt(...); \
    \
    BOOST_STATIC_CONSTANT(bool,value=(!boost::function_types::is_function<BOOST_TTI_DETAIL_TP_TYPE>::value) && (sizeof(chkt<BOOST_TTI_DETAIL_TP_T>(BOOST_TTI_DETAIL_NULLPTR))==sizeof(::boost::type_traits::yes_type))); \
    \
    typedef boost::mpl::bool_<value> type; \
    }; \
/**/

#endif // defined(BOOST_MSVC)

#endif // BOOST_TTI_DETAIL_STATIC_MEM_DATA_HPP
