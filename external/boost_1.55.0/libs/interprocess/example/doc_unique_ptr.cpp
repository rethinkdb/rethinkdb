//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

//[doc_unique_ptr
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <cassert>
//<-
#include "../test/get_process_id_name.hpp"
//->

using namespace boost::interprocess;

//This is type of the object we'll allocate dynamically
struct MyType
{
   MyType(int number = 0)
      :  number_(number)
   {}
   int number_;
};

//This is the type of a unique pointer to the previous type
//that will be built in the mapped file
typedef managed_unique_ptr<MyType, managed_mapped_file>::type unique_ptr_type;

//Define containers of unique pointer. Unique pointer simplifies object management
typedef vector
   < unique_ptr_type
   , allocator<unique_ptr_type, managed_mapped_file::segment_manager>
   > unique_ptr_vector_t;

typedef list
   < unique_ptr_type
   , allocator<unique_ptr_type, managed_mapped_file::segment_manager>
   > unique_ptr_list_t;

int main ()
{
   //Define file names
   //<-
   #if 1
   std::string mapped_file(boost::interprocess::ipcdetail::get_temporary_path());
   mapped_file += "/"; mapped_file += test::get_process_id_name();
   const char *MappedFile = mapped_file.c_str();
   #else
   //->
   const char *MappedFile  = "MyMappedFile";
   //<-
   #endif
   //->

   //Destroy any previous file with the name to be used.
   struct file_remove
   {
      file_remove(const char *MappedFile)
         : MappedFile_(MappedFile) { file_mapping::remove(MappedFile_); }
      ~file_remove(){ file_mapping::remove(MappedFile_); }
      const char *MappedFile_;
   } remover(MappedFile);
   {
      managed_mapped_file file(create_only, MappedFile, 65536);

      //Construct an object in the file and
      //pass ownership to this local unique pointer
      unique_ptr_type local_unique_ptr (make_managed_unique_ptr
         (file.construct<MyType>("unique object")(), file));
      assert(local_unique_ptr.get() != 0);

      //Reset the unique pointer. The object is automatically destroyed
      local_unique_ptr.reset();
      assert(file.find<MyType>("unique object").first == 0);

      //Now create a vector of unique pointers
      unique_ptr_vector_t *unique_vector =
         file.construct<unique_ptr_vector_t>("unique vector")(file.get_segment_manager());

      //Speed optimization
      unique_vector->reserve(100);

      //Now insert all values
      for(int i = 0; i < 100; ++i){
         unique_ptr_type p(make_managed_unique_ptr(file.construct<MyType>(anonymous_instance)(i), file));
         unique_vector->push_back(boost::move(p));
         assert(unique_vector->back()->number_ == i);
      }

      //Now create a list of unique pointers
      unique_ptr_list_t *unique_list =
         file.construct<unique_ptr_list_t>("unique list")(file.get_segment_manager());

      //Pass ownership of all values to the list
      for(int i = 99; !unique_vector->empty(); --i){
         unique_list->push_front(boost::move(unique_vector->back()));
         //The unique ptr of the vector is now empty...
         assert(unique_vector->back() == 0);
         unique_vector->pop_back();
         //...and the list has taken ownership of the value
         assert(unique_list->front() != 0);
         assert(unique_list->front()->number_ == i);
      }
      assert(unique_list->size() == 100);

      //Now destroy the empty vector.
      file.destroy_ptr(unique_vector);
      //The mapped file is unmapped here. Objects have been flushed to disk
   }
   {
      //Reopen the mapped file and find again the list
      managed_mapped_file file(open_only, MappedFile);

      unique_ptr_list_t   *unique_list =
         file.find<unique_ptr_list_t>("unique list").first;
      assert(unique_list);
      assert(unique_list->size() == 100);

      unique_ptr_list_t::const_iterator list_it = unique_list->begin();
      for(int i = 0; i < 100; ++i, ++list_it){
         assert((*list_it)->number_ == i);
      }

      //Now destroy the list. All elements will be automatically deallocated.
      file.destroy_ptr(unique_list);
   }
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
