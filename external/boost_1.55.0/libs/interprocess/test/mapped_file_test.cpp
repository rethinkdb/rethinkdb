//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/detail/file_wrapper.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include "named_creation_template.hpp"
#include <cstdio>
#include <cstring>
#include <string>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include "get_process_id_name.hpp"

using namespace boost::interprocess;

static const std::size_t FileSize = 1000;
inline std::string get_filename()
{
   std::string ret (ipcdetail::get_temporary_path());
   ret += "/";
   ret += test::get_process_id_name();
   return ret;
}

struct file_destroyer
{
   ~file_destroyer()
   {
      //The last destructor will destroy the file
      file_mapping::remove(get_filename().c_str());
   }
};

//This wrapper is necessary to have a common constructor
//in generic named_creation_template functions
class mapped_file_creation_test_wrapper
   : public file_destroyer
   , public boost::interprocess::ipcdetail::managed_open_or_create_impl
      <boost::interprocess::ipcdetail::file_wrapper, 0, true, false>
{
   typedef boost::interprocess::ipcdetail::managed_open_or_create_impl
      <boost::interprocess::ipcdetail::file_wrapper, 0, true, false> mapped_file;
   public:
   mapped_file_creation_test_wrapper(boost::interprocess::create_only_t)
      :  mapped_file(boost::interprocess::create_only, get_filename().c_str(), FileSize, read_write, 0, permissions())
   {}

   mapped_file_creation_test_wrapper(boost::interprocess::open_only_t)
      :  mapped_file(boost::interprocess::open_only, get_filename().c_str(), read_write, 0)
   {}

   mapped_file_creation_test_wrapper(boost::interprocess::open_or_create_t)
      :  mapped_file(boost::interprocess::open_or_create, get_filename().c_str(), FileSize, read_write, 0, permissions())
   {}
};

int main ()
{
   typedef boost::interprocess::ipcdetail::managed_open_or_create_impl
      <boost::interprocess::ipcdetail::file_wrapper, 0, true, false> mapped_file;
   file_mapping::remove(get_filename().c_str());
   test::test_named_creation<mapped_file_creation_test_wrapper>();

   //Create and get name, size and address
   {
      mapped_file file1(create_only, get_filename().c_str(), FileSize, read_write, 0, permissions());

      //Overwrite all memory
      std::memset(file1.get_user_address(), 0, file1.get_user_size());

      //Now test move semantics
      mapped_file move_ctor(boost::move(file1));
      mapped_file move_assign;
      move_assign = boost::move(move_ctor);
   }
//   file_mapping::remove(get_filename().c_str());
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
