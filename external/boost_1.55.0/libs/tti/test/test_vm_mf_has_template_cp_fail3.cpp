
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
  
  // Wrong template types
  
  BOOST_MPL_ASSERT((boost::tti::mf_has_template_check_params
                      <
                      WrongParametersForMP<_>,
                      boost::mpl::identity<AnotherType>
                      >
                  ));
  
#else
  
  BOOST_MPL_ASSERT((boost::mpl::false_));
  
#endif // BOOST_PP_VARIADICS

  return 0;
  
  }
