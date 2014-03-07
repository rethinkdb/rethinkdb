
//  (C) Copyright Edward Diener 2012,2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_FUNCTION_HPP)
#define BOOST_TTI_DETAIL_FUNCTION_HPP

#include <boost/config.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/tti/detail/dmem_fun.hpp>
#include <boost/tti/detail/dstatic_mem_fun.hpp>
#include <boost/tti/gen/namespace_gen.hpp>

#define BOOST_TTI_DETAIL_TRAIT_HAS_FUNCTION(trait,name) \
  BOOST_TTI_DETAIL_TRAIT_HAS_CALL_TYPES_MEMBER_FUNCTION(trait,name) \
  BOOST_TTI_DETAIL_TRAIT_IMPL_HAS_STATIC_MEMBER_FUNCTION(trait,name) \
  template<class BOOST_TTI_DETAIL_TP_T,class BOOST_TTI_DETAIL_TP_R,class BOOST_TTI_DETAIL_TP_FS,class BOOST_TTI_DETAIL_TP_TAG> \
  struct BOOST_PP_CAT(trait,_detail_hf) \
    { \
    \
    typedef typename \
    BOOST_PP_CAT(trait,_detail_call_types)<BOOST_TTI_DETAIL_TP_T,BOOST_TTI_DETAIL_TP_R,BOOST_TTI_DETAIL_TP_FS,BOOST_TTI_DETAIL_TP_TAG>::type hmftype; \
    \
    typedef typename \
    BOOST_PP_CAT(trait,_detail_ihsmf) \
      < \
      BOOST_TTI_DETAIL_TP_T, \
      typename BOOST_TTI_NAMESPACE::detail::tfunction_seq<BOOST_TTI_DETAIL_TP_R,BOOST_TTI_DETAIL_TP_FS,BOOST_TTI_DETAIL_TP_TAG>::type \
      >::type hsmftype; \
    \
    BOOST_STATIC_CONSTANT \
      ( \
      bool, \
      value = hmftype::value || hsmftype::value \
      ); \
    \
    typedef boost::mpl::bool_<value> type; \
    }; \
/**/

#endif // BOOST_TTI_DETAIL_FUNCTION_HPP
