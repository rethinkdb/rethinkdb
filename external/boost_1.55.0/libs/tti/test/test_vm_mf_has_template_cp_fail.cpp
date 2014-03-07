
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#include "test_vm_mf_has_template_cp.hpp"
#include <boost/mpl/assert.hpp>
#include <boost/tti/mf/mf_has_template_check_params.hpp>

int main()
  {
  
#if BOOST_PP_VARIADICS

  using namespace boost::mpl::placeholders;
  
  // Template does not exist
  
  BOOST_MPL_ASSERT((boost::tti::mf_has_template_check_params
                      <
                      BOOST_TTI_VM_HAS_TEMPLATE_CHECK_PARAMS_GEN(TemplateNotExist)<_>,
                      boost::tti::mf_member_type
                        <
                        BOOST_TTI_MEMBER_TYPE_GEN(CType)<_>,
                        MT_BType<AType>
                        >
                      >
                  ));
  
#else
  
  BOOST_MPL_ASSERT((boost::mpl::false_));
  
#endif // BOOST_PP_VARIADICS

  return 0;
  
  }
