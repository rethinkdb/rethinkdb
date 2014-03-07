/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2006-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_TRIVIAL_VALUE_TRAITS_HPP
#define BOOST_INTRUSIVE_TRIVIAL_VALUE_TRAITS_HPP

#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/pointer_traits.hpp>

namespace boost {
namespace intrusive {

//!This value traits template is used to create value traits
//!from user defined node traits where value_traits::value_type and
//!node_traits::node should be equal
template<class NodeTraits, link_mode_type LinkMode = normal_link>
struct trivial_value_traits
{
   typedef NodeTraits                                          node_traits;
   typedef typename node_traits::node_ptr                      node_ptr;
   typedef typename node_traits::const_node_ptr                const_node_ptr;
   typedef typename node_traits::node                          value_type;
   typedef node_ptr                                            pointer;
   typedef const_node_ptr                                      const_pointer;
   static const link_mode_type link_mode = LinkMode;
   static node_ptr       to_node_ptr (value_type &value)
      {  return pointer_traits<node_ptr>::pointer_to(value);  }
   static const_node_ptr to_node_ptr (const value_type &value)
      {  return pointer_traits<const_node_ptr>::pointer_to(value);  }
   static const pointer  &      to_value_ptr(const node_ptr &n)        {  return n; }
   static const const_pointer  &to_value_ptr(const const_node_ptr &n)  {  return n; }
};

} //namespace intrusive
} //namespace boost

#endif //BOOST_INTRUSIVE_TRIVIAL_VALUE_TRAITS_HPP
