
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_vm_mf_has_template_cp.hpp"
#include <boost/mpl/assert.hpp>
#include <boost/tti/mf/mf_has_template_check_params.hpp>
#include <boost/tti/mf/mf_member_type.hpp>

int main()
  {
  
#if BOOST_PP_VARIADICS

  using namespace boost::mpl::placeholders;
  
  // You can always instantiate without compiler errors
  
  boost::tti::mf_has_template_check_params
                <
                boost::mpl::quote1<BOOST_TTI_VM_HAS_TEMPLATE_CHECK_PARAMS_GEN(TemplateNotExist)>,
                boost::tti::mf_member_type
                  <
                  BOOST_TTI_MEMBER_TYPE_GEN(CType)<_>,
                  MT_BType<AType>
                  >
                > aVar;
  
  // Compile time asserts
  
  BOOST_MPL_ASSERT((boost::tti::mf_has_template_check_params
                      <
                      HT_Str<_>,
                      BOOST_TTI_MEMBER_TYPE_GEN(AStructType)<AType>
                      >
                  ));
  
  BOOST_MPL_ASSERT((boost::tti::mf_has_template_check_params
                      <
                      BOOST_TTI_VM_HAS_TEMPLATE_CHECK_PARAMS_GEN(AnotherMemberTemplate)<_>,
                      boost::mpl::identity<AType>
                      >
                  ));
  
  BOOST_MPL_ASSERT((boost::tti::mf_has_template_check_params
                      <
                      BOOST_TTI_VM_HAS_TEMPLATE_CHECK_PARAMS_GEN(CTManyParameters)<_>,
                      boost::tti::mf_member_type
                        <
                        boost::mpl::quote1<BOOST_TTI_MEMBER_TYPE_GEN(CType)>,
                        MT_BType<AType>
                        >
                      >
                  ));
  
  BOOST_MPL_ASSERT((boost::tti::mf_has_template_check_params
                      <
                      boost::mpl::quote1<HT_Str>,
                      BOOST_TTI_MEMBER_TYPE_GEN(AStructType)<AType>
                      >
                  ));
  
  BOOST_MPL_ASSERT((boost::tti::mf_has_template_check_params
                      <
                      boost::mpl::quote1<BOOST_TTI_VM_HAS_TEMPLATE_CHECK_PARAMS_GEN(AnotherMemberTemplate)>,
                      boost::mpl::identity<AType>
                      >
                  ));
  
  BOOST_MPL_ASSERT((boost::tti::mf_has_template_check_params
                      <
                      boost::mpl::quote1<BOOST_TTI_VM_HAS_TEMPLATE_CHECK_PARAMS_GEN(CTManyParameters)>,
                      boost::tti::mf_member_type
                        <
                        BOOST_TTI_MEMBER_TYPE_GEN(CType)<_>,
                        MT_BType<AType>
                        >
                      >
                  ));
  
#endif // BOOST_PP_VARIADICS

  return 0;
  
  }
