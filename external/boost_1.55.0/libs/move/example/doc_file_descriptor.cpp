//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2008-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/move/detail/config_begin.hpp>
#include <cassert>

//[file_descriptor_def

#include <boost/move/utility.hpp>
#include <stdexcept>

class file_descriptor
{
   //<-
   int operating_system_open_file(const char *)
   {
      return 1;
   }

   void operating_system_close_file(int fd)
   {  (void)fd;   assert(fd != 0); }
   //->
   int os_descr_;

   private:
   BOOST_MOVABLE_BUT_NOT_COPYABLE(file_descriptor)

   public:
   explicit file_descriptor(const char *filename)              //Constructor
      : os_descr_(operating_system_open_file(filename))
   {  if(!os_descr_) throw std::runtime_error("file not found");  }

   ~file_descriptor()                                          //Destructor
   {  if(os_descr_)  operating_system_close_file(os_descr_);  }

   file_descriptor(BOOST_RV_REF(file_descriptor) x)            // Move ctor
      :  os_descr_(x.os_descr_)
   {  x.os_descr_ = 0;  }      

   file_descriptor& operator=(BOOST_RV_REF(file_descriptor) x) // Move assign
   {
      if(os_descr_) operating_system_close_file(os_descr_);
      os_descr_   = x.os_descr_;
      x.os_descr_ = 0;
      return *this;
   }

   bool empty() const   {  return os_descr_ == 0;  }
};

//]

//[file_descriptor_example
#include <boost/container/vector.hpp>
#include <cassert>

//Remember: 'file_descriptor' is NOT copyable, but it
//can be returned from functions thanks to move semantics
file_descriptor create_file_descriptor(const char *filename)
{  return file_descriptor(filename);  }

int main()
{
   //Open a file obtaining its descriptor, the temporary
   //returned from 'create_file_descriptor' is moved to 'fd'.
   file_descriptor fd = create_file_descriptor("filename");
   assert(!fd.empty());

   //Now move fd into a vector
   boost::container::vector<file_descriptor> v;
   v.push_back(boost::move(fd));

   //Check ownership has been transferred
   assert(fd.empty());
   assert(!v[0].empty());

   //Compilation error if uncommented since file_descriptor is not copyable
   //and vector copy construction requires value_type's copy constructor:
   //boost::container::vector<file_descriptor> v2(v);
   return 0;
}
//]

#include <boost/move/detail/config_end.hpp>
