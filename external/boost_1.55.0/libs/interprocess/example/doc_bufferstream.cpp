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
//[doc_bufferstream
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <vector>
#include <iterator>
#include <cstddef>
//<-
#include "../test/get_process_id_name.hpp"
//->

using namespace boost::interprocess;

int main ()
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
   managed_shared_memory segment(create_only,test::get_process_id_name(), 65536);
   #else
   //->
   managed_shared_memory segment(create_only,
                                 "MySharedMemory",  //segment name
                                 65536);
   //<-
   #endif
   //->

   //Fill data
   std::vector<int> data;
   data.reserve(100);
   for(int i = 0; i < 100; ++i){
      data.push_back(i);
   }
   const std::size_t BufferSize = 100*5;

   //Allocate a buffer in shared memory to write data
   char *my_cstring =
      segment.construct<char>("MyCString")[BufferSize](0);
   bufferstream mybufstream(my_cstring, BufferSize);

   //Now write data to the buffer
   for(int i = 0; i < 100; ++i){
      mybufstream << data[i] << std::endl;
   }

   //Check there was no overflow attempt
   assert(mybufstream.good());

   //Extract all values from the shared memory string
   //directly to a vector.
   std::vector<int> data2;
   std::istream_iterator<int> it(mybufstream), itend;
   std::copy(it, itend, std::back_inserter(data2));

   //This extraction should have ended will fail error since
   //the numbers formatted in the buffer end before the end
   //of the buffer. (Otherwise it would trigger eofbit)
   assert(mybufstream.fail());

   //Compare data
   assert(std::equal(data.begin(), data.end(), data2.begin()));

   //Clear errors and rewind
   mybufstream.clear();
   mybufstream.seekp(0, std::ios::beg);

   //Now write again the data trying to do a buffer overflow
   for(int i = 0, m = data.size()*5; i < m; ++i){
      mybufstream << data[i%5] << std::endl;
   }

   //Now make sure badbit is active
   //which means overflow attempt.
   assert(!mybufstream.good());
   assert(mybufstream.bad());
   segment.destroy_ptr(my_cstring);
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
