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

#include <boost/config.hpp>

#ifdef BOOST_NO_EXCEPTIONS

//Interprocess does not support BOOST_NO_EXCEPTIONS so nothing to test here
int main()
{
   return 0;
}

#else //!BOOST_NO_EXCEPTIONS

//This is needed to allow concurrent test execution in
//several platforms. The shared memory must be unique
//for each process...
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <sstream>

const char *get_shared_memory_name()
{
   std::stringstream s;
   s << "process_" << boost::interprocess::ipcdetail::get_current_process_id();
   static std::string str = s.str();
   return str.c_str();
}

//[doc_offset_ptr_0
#include <boost/intrusive/list.hpp>
#include <boost/interprocess/offset_ptr.hpp>

using namespace boost::intrusive;
namespace ip = boost::interprocess;

class shared_memory_data
   //Declare the hook with an offset_ptr from Boost.Interprocess
   //to make this class compatible with shared memory
   :  public list_base_hook< void_pointer< ip::offset_ptr<void> > >
{
   int data_id_;
   public:

   int get() const   {  return data_id_;  }
   void set(int id)  {  data_id_ = id;    }
};
//]

//[doc_offset_ptr_1
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

//Definition of the shared memory friendly intrusive list
typedef list<shared_memory_data> intrusive_list_t;

int main()
{
   //Now create an intrusive list in shared memory:
   //nodes and the container itself must be created in shared memory
   const int MaxElem    = 100;
   const int ShmSize    = 50000;
   const char *ShmName  = get_shared_memory_name();
   {
      //Erase all old shared memory
      ip::shared_memory_object::remove(ShmName);
      ip::managed_shared_memory shm(ip::create_only, ShmName, ShmSize);

      //Create all nodes in shared memory using a shared memory vector
      //See Boost.Interprocess documentation for more information on this
      typedef ip::allocator
         < shared_memory_data, ip::managed_shared_memory::segment_manager>
            shm_allocator_t;
      typedef ip::vector<shared_memory_data, shm_allocator_t> shm_vector_t;
      shm_allocator_t shm_alloc(shm.get_segment_manager());
      shm_vector_t *pshm_vect =
         shm.construct<shm_vector_t>(ip::anonymous_instance)(shm_alloc);
      pshm_vect->resize(MaxElem);

      //Initialize all the nodes
      for(int i = 0; i < MaxElem; ++i)    (*pshm_vect)[i].set(i);

      //Now create the shared memory intrusive list
      intrusive_list_t *plist = shm.construct<intrusive_list_t>(ip::anonymous_instance)();

      //Insert objects stored in shared memory vector in the intrusive list
      plist->insert(plist->end(), pshm_vect->begin(), pshm_vect->end());

      //Check all the inserted nodes
      int checker = 0;
      for( intrusive_list_t::const_iterator it = plist->begin(), itend(plist->end())
         ; it != itend; ++it, ++checker){
         if(it->get() != checker)   return false;
      }

      //Now delete the list and after that, the nodes
      shm.destroy_ptr(plist);
      shm.destroy_ptr(pshm_vect);
   }
   ip::shared_memory_object::remove(ShmName);
   return 0;
}
//]

#endif //BOOST_NO_EXCEPTIONS

