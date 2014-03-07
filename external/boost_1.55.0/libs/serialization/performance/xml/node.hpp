/* /libs/serialization/xml_performance/node.hpp ********************************

(C) Copyright 2010 Bryce Lelbach

Use, modification and distribution is subject to the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at 
http://www.boost.org/LICENSE_1_0.txt)

*******************************************************************************/

#if !defined(BOOST_SERIALIZATION_XML_PERFORMANCE_NODE_HPP)
#define BOOST_SERIALIZATION_XML_PERFORMANCE_NODE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
  #pragma once
#endif

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/version.hpp>

#include "macro.hpp"

namespace boost {
namespace archive {
namespace xml {

struct unused_type { };

template<
  typename T,
  BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(
    BOOST_PP_SUB(BSL_NODE_MAX, 1), typename T, unused_type
  )
> struct node;

BOOST_PP_REPEAT_FROM_TO(1, BSL_NODE_MAX, BSL_NODE_DECL, _)

template<BOOST_PP_ENUM_PARAMS(BSL_NODE_MAX, typename T)>
struct node {
  BOOST_PP_REPEAT(BSL_NODE_MAX, BSL_NODE_DECL_MEMBER, _)

  template<class ARC>                                   
  void serialize (ARC& ar, const unsigned int) {        
    ar                                                  
      BOOST_PP_REPEAT(BSL_NODE_MAX, BSL_NODE_SERIALIZE, _)        
    ;                                                   
  }                                                     
                                                        
  BSL_NODE_xDECL_CTOR()                                      
                                                        
  node (BOOST_PP_ENUM_BINARY_PARAMS(BSL_NODE_MAX, T, p)):     
    BOOST_PP_REPEAT(BSL_NODE_MAX, BSL_NODE_INIT_LIST, _) 
  { }
};                                                      

} // xml
} // archive
} // boost

#endif // BOOST_SERIALIZATION_XML_PERFORMANCE_NODE_HPP

