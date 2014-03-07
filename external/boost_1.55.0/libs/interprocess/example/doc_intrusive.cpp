//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
//[doc_intrusive
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/smart_ptr/intrusive_ptr.hpp>
//<-
#include "../test/get_process_id_name.hpp"
//->

using namespace boost::interprocess;

namespace N {

//A class that has an internal reference count
class reference_counted_class
{
   private:
   //Non-copyable
   reference_counted_class(const reference_counted_class  &);
   //Non-assignable
   reference_counted_class & operator=(const reference_counted_class &);
   //A typedef to save typing
   typedef managed_shared_memory::segment_manager segment_manager;
   //This is the reference count
   unsigned int m_use_count;
   //The segment manager allows deletion from shared memory segment
   offset_ptr<segment_manager> mp_segment_manager;

   public:
   //Constructor
   reference_counted_class(segment_manager *s_mngr)
   : m_use_count(0), mp_segment_manager(s_mngr){}
   //Destructor
   ~reference_counted_class(){}

   public:
   //Returns the reference count
   unsigned int use_count() const
   {  return m_use_count;   }

   //Adds a reference
   inline friend void intrusive_ptr_add_ref(reference_counted_class * p)
   {  ++p->m_use_count; }

   //Releases a reference
   inline friend void intrusive_ptr_release(reference_counted_class * p)
   {  if(--p->m_use_count == 0)  p->mp_segment_manager->destroy_ptr(p); }
};

}  //namespace N {

//A class that has an intrusive pointer to reference_counted_class
class intrusive_ptr_owner
{
   typedef intrusive_ptr<N::reference_counted_class,
                           offset_ptr<void> > intrusive_ptr_t;
   intrusive_ptr_t m_intrusive_ptr;

   public:
   //Takes a pointer to the reference counted class
   intrusive_ptr_owner(N::reference_counted_class *ptr)
      : m_intrusive_ptr(ptr){}
};

int main()
{
   //Remove shared memory on construction and destruction
   struct shm_remove
   {
   //<-
   #if 1
      shm_remove() { shared_memory_object::remove(test::get_process_id_name()); }
      ~shm_remove(){ shared_memory_object::remove(test::get_process_id_name()); }
   #else
   //->
      shm_remove() { shared_memory_object::remove("MySharedMemory"); }
      ~shm_remove(){ shared_memory_object::remove("MySharedMemory"); }
   //<-
   #endif
   //->
   } remover;
   //<-
   (void)remover;
   //->

   //Create shared memory
   //<-
   #if 1
   managed_shared_memory shmem(create_only, test::get_process_id_name(), 10000);
   #else
   //->
   managed_shared_memory shmem(create_only, "MySharedMemory", 10000);
   //<-
   #endif
   //->

   //Create the unique reference counted object in shared memory
   N::reference_counted_class *ref_counted =
      shmem.construct<N::reference_counted_class>
         ("ref_counted")(shmem.get_segment_manager());

   //Create an array of ten intrusive pointer owners in shared memory
   intrusive_ptr_owner *intrusive_owner_array =
      shmem.construct<intrusive_ptr_owner>
         (anonymous_instance)[10](ref_counted);

   //Now test that reference count is ten
   if(ref_counted->use_count() != 10)
      return 1;

   //Now destroy the array of intrusive pointer owners
   //This should destroy every intrusive_ptr and because of
   //that reference_counted_class will be destroyed
   shmem.destroy_ptr(intrusive_owner_array);

   //Now the reference counted object should have been destroyed
   if(shmem.find<intrusive_ptr_owner>("ref_counted").first)
      return 1;
   //Success!
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
