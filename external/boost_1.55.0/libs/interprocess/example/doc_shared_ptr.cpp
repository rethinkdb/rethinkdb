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

//[doc_shared_ptr
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/smart_ptr/shared_ptr.hpp>
#include <boost/interprocess/smart_ptr/weak_ptr.hpp>
#include <cassert>
//<-
#include "../test/get_process_id_name.hpp"
//->

using namespace boost::interprocess;

//This is type of the object we want to share
struct type_to_share
{};

//This is the type of a shared pointer to the previous type
//that will be built in the mapped file
typedef managed_shared_ptr<type_to_share, managed_mapped_file>::type shared_ptr_type;
typedef managed_weak_ptr<type_to_share, managed_mapped_file>::type   weak_ptr_type;

//This is a type holding a shared pointer
struct shared_ptr_owner
{
   shared_ptr_owner(const shared_ptr_type &other_shared_ptr)
      : shared_ptr_(other_shared_ptr)
   {}

   shared_ptr_owner(const shared_ptr_owner &other_owner)
      : shared_ptr_(other_owner.shared_ptr_)
   {}

   shared_ptr_type shared_ptr_;
   //...
};

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

      //Construct the shared type in the file and
      //pass ownership to this local shared pointer
      shared_ptr_type local_shared_ptr = make_managed_shared_ptr
         (file.construct<type_to_share>("object to share")(), file);
      assert(local_shared_ptr.use_count() == 1);

      //Share ownership of the object between local_shared_ptr and a new "owner1"
      shared_ptr_owner *owner1 =
         file.construct<shared_ptr_owner>("owner1")(local_shared_ptr);
      assert(local_shared_ptr.use_count() == 2);

      //local_shared_ptr releases object ownership
      local_shared_ptr.reset();
      assert(local_shared_ptr.use_count() == 0);
      assert(owner1->shared_ptr_.use_count() == 1);

      //Share ownership of the object between "owner1" and a new "owner2"
      shared_ptr_owner *owner2 =
         file.construct<shared_ptr_owner>("owner2")(*owner1);
      assert(owner1->shared_ptr_.use_count() == 2);
      assert(owner2->shared_ptr_.use_count() == 2);
      assert(owner1->shared_ptr_.get() == owner2->shared_ptr_.get());

      //The mapped file is unmapped here. Objects have been flushed to disk
   }
   {
      //Reopen the mapped file and find again all owners
      managed_mapped_file file(open_only, MappedFile);

      shared_ptr_owner *owner1 = file.find<shared_ptr_owner>("owner1").first;
      shared_ptr_owner *owner2 = file.find<shared_ptr_owner>("owner2").first;
      assert(owner1 && owner2);

      //Check everything is as expected
      assert(file.find<type_to_share>("object to share").first != 0);
      assert(owner1->shared_ptr_.use_count() == 2);
      assert(owner2->shared_ptr_.use_count() == 2);
      assert(owner1->shared_ptr_.get() == owner2->shared_ptr_.get());

      //Now destroy one of the owners, the reference count drops.
      file.destroy_ptr(owner1);
      assert(owner2->shared_ptr_.use_count() == 1);

      //Create a weak pointer
      weak_ptr_type local_observer1(owner2->shared_ptr_);
      assert(local_observer1.use_count() == owner2->shared_ptr_.use_count());

      {  //Create a local shared pointer from the weak pointer
      shared_ptr_type local_shared_ptr = local_observer1.lock();
      assert(local_observer1.use_count() == owner2->shared_ptr_.use_count());
      assert(local_observer1.use_count() == 2);
      }

      //Now destroy the remaining owner. "object to share" will be destroyed
      file.destroy_ptr(owner2);
      assert(file.find<type_to_share>("object to share").first == 0);

      //Test observer
      assert(local_observer1.expired());
      assert(local_observer1.use_count() == 0);

      //The reference count will be deallocated when all weak pointers
      //disappear. After that, the file is unmapped.
   }
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
