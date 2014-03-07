
//  (C) Copyright Edward Diener 2012
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_DATA_HPP)
#define BOOST_TTI_DETAIL_DATA_HPP

#include <boost/config.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/tti/detail/dmem_data.hpp>
#include <boost/tti/detail/dstatic_mem_data.hpp>
#include <boost/tti/gen/namespace_gen.hpp>
#include <boost/type_traits/remove_const.hpp>

#define BOOST_TTI_DETAIL_TRAIT_HAS_DATA(trait,name) \
  BOOST_TTI_DETAIL_TRAIT_HAS_MEMBER_DATA(trait,name) \
  BOOST_TTI_DETAIL_TRAIT_HAS_STATIC_MEMBER_DATA(trait,name) \
  template<class BOOST_TTI_DETAIL_TP_ET,class BOOST_TTI_DETAIL_TP_DT> \
  struct BOOST_PP_CAT(trait,_detail_hd) \
    { \
    \
    typedef typename \
    BOOST_PP_CAT(trait,_detail_hmd) \
      < \
      typename BOOST_TTI_NAMESPACE::detail::ptmd<BOOST_TTI_DETAIL_TP_ET,BOOST_TTI_DETAIL_TP_DT>::type, \
      typename boost::remove_const<BOOST_TTI_DETAIL_TP_ET>::type \
      >::type hmdtype; \
    \
    typedef typename \
    BOOST_PP_CAT(trait,_detail_hsd)<BOOST_TTI_DETAIL_TP_ET,BOOST_TTI_DETAIL_TP_DT>::type hsdtype; \
    \
    BOOST_STATIC_CONSTANT \
      ( \
      bool, \
      value = hmdtype::value || hsdtype::value \
      ); \
    \
    typedef boost::mpl::bool_<value> type; \
    }; \
/**/

#endif // BOOST_TTI_DETAIL_DATA_HPP
