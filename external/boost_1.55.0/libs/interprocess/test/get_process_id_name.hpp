//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_GET_PROCESS_ID_NAME_HPP
#define BOOST_INTERPROCESS_GET_PROCESS_ID_NAME_HPP

#include <boost/config.hpp>
#include <string>    //std::string
#include <sstream>   //std::stringstream
#include <boost/interprocess/detail/os_thread_functions.hpp>

namespace boost{
namespace interprocess{
namespace test{

inline void get_process_id_name(std::string &str)
{
   std::stringstream sstr;
   sstr << "process_" << boost::interprocess::ipcdetail::get_current_process_id() << std::ends;
   str = sstr.str().c_str();
}

inline void get_process_id_ptr_name(std::string &str, const void *ptr)
{
   std::stringstream sstr;
   sstr << "process_" << boost::interprocess::ipcdetail::get_current_process_id() << "_" << ptr << std::ends;
   str = sstr.str().c_str();
}

inline const char *get_process_id_name()
{
   static std::string str;
   get_process_id_name(str);
   return str.c_str();
}

inline const char *get_process_id_ptr_name(void *ptr)
{
   static std::string str;
   get_process_id_ptr_name(str, ptr);
   return str.c_str();
}

inline const char *add_to_process_id_name(const char *name)
{
   static std::string str;
   get_process_id_name(str);
   str += name;
   return str.c_str();
}

inline const char *add_to_process_id_ptr_name(const char *name, void *ptr)
{
   static std::string str;
   get_process_id_ptr_name(str, ptr);
   str += name;
   return str.c_str();
}

}  //namespace test{
}  //namespace interprocess{
}  //namespace boost{

#endif //#ifndef BOOST_INTERPROCESS_GET_PROCESS_ID_NAME_HPP
