//  (C) Copyright John Maddock 2003. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.


#define BOOST_CONFIG_SOURCE

#include "link_test.hpp"
#include <iostream>
#include <iomanip>

bool BOOST_CONFIG_DECL check_options(
                   bool m_dyn_link,
                   bool m_dyn_rtl,
                   bool m_has_threads,
                   bool m_debug,
                   bool m_stlp_debug)
{
   if(m_dyn_link != dyn_link)
   {
      std::cout << "Dynamic link options do not match" << std::endl;
      std::cout << "Application setting = " << m_dyn_link << " Library setting = " << dyn_link << std::endl;
      return false;
   }
   if(m_dyn_rtl != dyn_rtl)
   {
      std::cout << "Runtime library options do not match" << std::endl;
      std::cout << "Application setting = " << m_dyn_rtl << " Library setting = " << dyn_rtl << std::endl;
      return false;
   }
   if(m_has_threads != has_threads)
   {
      std::cout << "Threading options do not match" << std::endl;
      std::cout << "Application setting = " << m_has_threads << " Library setting = " << has_threads << std::endl;
      return false;
   }
   if(m_debug != debug)
   {
      std::cout << "Debug options do not match" << std::endl;
      std::cout << "Application setting = " << m_debug << " Library setting = " << debug << std::endl;
      return false;
   }
   if(m_stlp_debug != stl_debug)
   {
      std::cout << "STLPort debug options do not match" << std::endl;
      std::cout << "Application setting = " << m_stlp_debug << " Library setting = " << stl_debug << std::endl;
      return false;
   }
   return true;
}

