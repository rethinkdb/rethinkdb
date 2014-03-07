//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_TEST_MEMORY_ALGORITHM_TEST_TEMPLATE_HEADER
#define BOOST_INTERPROCESS_TEST_MEMORY_ALGORITHM_TEST_TEMPLATE_HEADER

#include <boost/interprocess/detail/config_begin.hpp>
#include <vector>
#include <iostream>
#include <new>
#include <utility>
#include <cstring>   //std::memset
#include <boost/interprocess/containers/vector.hpp>

namespace boost { namespace interprocess { namespace test {

enum deallocation_type { DirectDeallocation, InverseDeallocation, MixedDeallocation, EndDeallocationType };

//This test allocates until there is no more memory
//and after that deallocates all in the inverse order
template<class Allocator>
bool test_allocation(Allocator &a)
{
   for( deallocation_type t = DirectDeallocation
      ; t != EndDeallocationType
      ; t = (deallocation_type)((int)t + 1)){
      std::vector<void*> buffers;
	  typename Allocator::size_type free_memory = a.get_free_memory();

      for(int i = 0; true; ++i){
         void *ptr = a.allocate(i, std::nothrow);
         if(!ptr)
            break;
		 std::size_t size = a.size(ptr);
         std::memset(ptr, 0, size);
         buffers.push_back(ptr);
      }

      switch(t){
         case DirectDeallocation:
         {
            for(int j = 0, max = (int)buffers.size()
               ;j < max
               ;++j){
               a.deallocate(buffers[j]);
            }
         }
         break;
         case InverseDeallocation:
         {
            for(int j = (int)buffers.size()
               ;j--
               ;){
               a.deallocate(buffers[j]);
            }
         }
         break;
         case MixedDeallocation:
         {
            for(int j = 0, max = (int)buffers.size()
               ;j < max
               ;++j){
               int pos = (j%4)*((int)buffers.size())/4;
               a.deallocate(buffers[pos]);
               buffers.erase(buffers.begin()+pos);
            }
         }
         break;
         default:
         break;
      }
      bool ok = free_memory == a.get_free_memory() &&
               a.all_memory_deallocated() && a.check_sanity();
      if(!ok)  return ok;
   }
   return true;
}

//This test allocates until there is no more memory
//and after that tries to shrink all the buffers to the
//half of the original size
template<class Allocator>
bool test_allocation_shrink(Allocator &a)
{
   std::vector<void*> buffers;

   //Allocate buffers with extra memory
   for(int i = 0; true; ++i){
      void *ptr = a.allocate(i*2, std::nothrow);
      if(!ptr)
         break;
	  std::size_t size = a.size(ptr);
      std::memset(ptr, 0, size);
      buffers.push_back(ptr);
   }

   //Now shrink to half
   for(int i = 0, max = (int)buffers.size()
      ;i < max
      ; ++i){
      typename Allocator::size_type received_size;
      if(a.template allocation_command<char>
         ( boost::interprocess::shrink_in_place | boost::interprocess::nothrow_allocation, i*2
         , i, received_size, static_cast<char*>(buffers[i])).first){
         if(received_size > std::size_t(i*2)){
            return false;
         }
         if(received_size < std::size_t(i)){
            return false;
         }
		 std::memset(buffers[i], 0, a.size(buffers[i]));
      }
   }

   //Deallocate it in non sequential order
   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      int pos = (j%4)*((int)buffers.size())/4;
      a.deallocate(buffers[pos]);
      buffers.erase(buffers.begin()+pos);
   }

   return a.all_memory_deallocated() && a.check_sanity();
}

//This test allocates until there is no more memory
//and after that tries to expand all the buffers to
//avoid the wasted internal fragmentation
template<class Allocator>
bool test_allocation_expand(Allocator &a)
{
   std::vector<void*> buffers;

   //Allocate buffers with extra memory
   for(int i = 0; true; ++i){
      void *ptr = a.allocate(i, std::nothrow);
      if(!ptr)
         break;
	  std::size_t size = a.size(ptr);
      std::memset(ptr, 0, size);
      buffers.push_back(ptr);
   }

   //Now try to expand to the double of the size
   for(int i = 0, max = (int)buffers.size()
      ;i < max
      ;++i){
      typename Allocator::size_type received_size;
      std::size_t min_size = i+1;
      std::size_t preferred_size = i*2;
      preferred_size = min_size > preferred_size ? min_size : preferred_size;

      while(a.template allocation_command<char>
         ( boost::interprocess::expand_fwd | boost::interprocess::nothrow_allocation, min_size
         , preferred_size, received_size, static_cast<char*>(buffers[i])).first){
         //Check received size is bigger than minimum
         if(received_size < min_size){
            return false;
         }
         //Now, try to expand further
		 min_size       = received_size+1;
         preferred_size = min_size*2;
      }
   }

   //Deallocate it in non sequential order
   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      int pos = (j%4)*((int)buffers.size())/4;
      a.deallocate(buffers[pos]);
      buffers.erase(buffers.begin()+pos);
   }

   return a.all_memory_deallocated() && a.check_sanity();
}

//This test allocates until there is no more memory
//and after that tries to expand all the buffers to
//avoid the wasted internal fragmentation
template<class Allocator>
bool test_allocation_shrink_and_expand(Allocator &a)
{
   std::vector<void*> buffers;
   std::vector<typename Allocator::size_type> received_sizes;
   std::vector<bool>        size_reduced;

   //Allocate buffers wand store received sizes
   for(int i = 0; true; ++i){
      typename Allocator::size_type received_size;
      void *ptr = a.template allocation_command<char>
         ( boost::interprocess::allocate_new | boost::interprocess::nothrow_allocation, i, i*2, received_size).first;
      if(!ptr){
         ptr = a.template allocation_command<char>
            ( boost::interprocess::allocate_new | boost::interprocess::nothrow_allocation, 1, i*2, received_size).first;
         if(!ptr)
            break;
      }
      buffers.push_back(ptr);
      received_sizes.push_back(received_size);
   }

   //Now shrink to half
   for(int i = 0, max = (int)buffers.size()
      ; i < max
      ; ++i){
      typename Allocator::size_type received_size;
      if(a.template allocation_command<char>
         ( boost::interprocess::shrink_in_place | boost::interprocess::nothrow_allocation, received_sizes[i]
         , i, received_size, static_cast<char*>(buffers[i])).first){
         if(received_size > std::size_t(received_sizes[i])){
            return false;
         }
         if(received_size < std::size_t(i)){
            return false;
         }
         size_reduced.push_back(received_size != received_sizes[i]);
      }
   }

   //Now try to expand to the original size
   for(int i = 0, max = (int)buffers.size()
      ;i < max
      ;++i){
      typename Allocator::size_type received_size;
      std::size_t request_size = received_sizes[i];
      if(a.template allocation_command<char>
         ( boost::interprocess::expand_fwd | boost::interprocess::nothrow_allocation, request_size
         , request_size, received_size, static_cast<char*>(buffers[i])).first){
         if(received_size != received_sizes[i]){
            return false;
         }
      }
      else{
         return false;
      }
   }

   //Deallocate it in non sequential order
   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      int pos = (j%4)*((int)buffers.size())/4;
      a.deallocate(buffers[pos]);
      buffers.erase(buffers.begin()+pos);
   }

   return a.all_memory_deallocated() && a.check_sanity();
}

//This test allocates until there is no more memory
//and after that deallocates the odd buffers to
//make room for expansions. The expansion will probably
//success since the deallocation left room for that.
template<class Allocator>
bool test_allocation_deallocation_expand(Allocator &a)
{
   std::vector<void*> buffers;

   //Allocate buffers with extra memory
   for(int i = 0; true; ++i){
      void *ptr = a.allocate(i, std::nothrow);
      if(!ptr)
         break;
      std::size_t size = a.size(ptr);
      std::memset(ptr, 0, size);
      buffers.push_back(ptr);
   }

   //Now deallocate the half of the blocks
   //so expand maybe can merge new free blocks
   for(int i = 0, max = (int)buffers.size()
      ;i < max
      ;++i){
      if(i%2){
         a.deallocate(buffers[i]);
         buffers[i] = 0;
      }
   }

   //Now try to expand to the double of the size
   for(int i = 0, max = (int)buffers.size()
      ;i < max
      ;++i){
      //
      if(buffers[i]){
         typename Allocator::size_type received_size;
         std::size_t min_size = i+1;
         std::size_t preferred_size = i*2;
         preferred_size = min_size > preferred_size ? min_size : preferred_size;

         while(a.template allocation_command<char>
            ( boost::interprocess::expand_fwd | boost::interprocess::nothrow_allocation, min_size
            , preferred_size, received_size, static_cast<char*>(buffers[i])).first){
            //Check received size is bigger than minimum
            if(received_size < min_size){
               return false;
            }
            //Now, try to expand further
            min_size       = received_size+1;
            preferred_size = min_size*2;
         }
      }
   }

   //Now erase null values from the vector
   buffers.erase( std::remove(buffers.begin(), buffers.end(), static_cast<void*>(0))
                , buffers.end());

   //Deallocate it in non sequential order
   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      int pos = (j%4)*((int)buffers.size())/4;
      a.deallocate(buffers[pos]);
      buffers.erase(buffers.begin()+pos);
   }

   return a.all_memory_deallocated() && a.check_sanity();
}

//This test allocates until there is no more memory
//and after that deallocates all except the last.
//If the allocation algorithm is a bottom-up algorithm
//the last buffer will be in the end of the segment.
//Then the test will start expanding backwards, until
//the buffer fills all the memory
template<class Allocator>
bool test_allocation_with_reuse(Allocator &a)
{
   //We will repeat this test for different sized elements
   for(int sizeof_object = 1; sizeof_object < 20; ++sizeof_object){
      std::vector<void*> buffers;

      //Allocate buffers with extra memory
      for(int i = 0; true; ++i){
         void *ptr = a.allocate(i*sizeof_object, std::nothrow);
         if(!ptr)
            break;
         std::size_t size = a.size(ptr);
         std::memset(ptr, 0, size);
         buffers.push_back(ptr);
      }

      //Now deallocate all except the latest
      //Now try to expand to the double of the sizeof_object
      for(int i = 0, max = (int)buffers.size() - 1
         ;i < max
         ;++i){
         a.deallocate(buffers[i]);
      }

      //Save the unique buffer and clear vector
      void *ptr = buffers.back();
      buffers.clear();

      //Now allocate with reuse
      typename Allocator::size_type received_size = 0;
      for(int i = 0; true; ++i){
         std::size_t min_size = (received_size + 1);
         std::size_t prf_size = (received_size + (i+1)*2);
         std::pair<void*, bool> ret = a.raw_allocation_command
            ( boost::interprocess::expand_bwd | boost::interprocess::nothrow_allocation, min_size
            , prf_size, received_size, static_cast<char*>(ptr), sizeof_object);
         if(!ret.first)
            break;
         //If we have memory, this must be a buffer reuse
         if(!ret.second)
            return 1;
         if(received_size < min_size)
            return 1;
         ptr = ret.first;
      }
      //There is only a single block so deallocate it
      a.deallocate(ptr);

      if(!a.all_memory_deallocated() || !a.check_sanity())
         return false;
   }
   return true;
}


//This test allocates memory with different alignments
//and checks returned memory is aligned.
template<class Allocator>
bool test_aligned_allocation(Allocator &a)
{
   //Allocate aligned buffers in a loop
   //and then deallocate it
   bool continue_loop = true;
   for(unsigned int i = 1; continue_loop; i <<= 1){
      for(unsigned int j = 1; true; j <<= 1){
         void *ptr = a.allocate_aligned(i-1, j, std::nothrow);
         if(!ptr){
            if(j == 1)
               continue_loop = false;
            break;
         }

         if(((std::size_t)ptr & (j - 1)) != 0)
            return false;
         a.deallocate(ptr);
         if(!a.all_memory_deallocated() || !a.check_sanity()){
            return false;
         }
      }
   }

   return a.all_memory_deallocated() && a.check_sanity();
}

//This test allocates memory with different alignments
//and checks returned memory is aligned.
template<class Allocator>
bool test_continuous_aligned_allocation(Allocator &a)
{
   std::vector<void*> buffers;
   //Allocate aligned buffers in a loop
   //and then deallocate it
   bool continue_loop = true;
   for(unsigned i = 1; continue_loop && i; i <<= 1){
      for(unsigned int j = 1; j; j <<= 1){
         for(bool any_allocated = false; 1;){
            void *ptr = a.allocate_aligned(i-1, j, std::nothrow);
            buffers.push_back(ptr);
            if(!ptr){
               if(j == 1 && !any_allocated){
                  continue_loop = false;
               }
               break;
            }
            else{
               any_allocated = true;
            }

            if(((std::size_t)ptr & (j - 1)) != 0)
               return false;
         }
         //Deallocate all
         for(unsigned int k = (int)buffers.size(); k--;){
            a.deallocate(buffers[k]);
         }
         buffers.clear();
         if(!a.all_memory_deallocated() && a.check_sanity())
            return false;
         if(!continue_loop)
            break;
      }
   }

   return a.all_memory_deallocated() && a.check_sanity();
}

//This test allocates memory, writes it with a non-zero value and
//tests zero_free_memory initializes to zero for the next allocation
template<class Allocator>
bool test_clear_free_memory(Allocator &a)
{
   std::vector<void*> buffers;

   //Allocate memory
   for(int i = 0; true; ++i){
      void *ptr = a.allocate(i, std::nothrow);
      if(!ptr)
         break;
      std::size_t size = a.size(ptr);
      std::memset(ptr, 1, size);
      buffers.push_back(ptr);
   }

   //Mark it
   for(int i = 0, max = buffers.size(); i < max; ++i){
      std::memset(buffers[i], 1, i);
   }

   //Deallocate all
   for(int j = (int)buffers.size()
      ;j--
      ;){
      a.deallocate(buffers[j]);
   }
   buffers.clear();

   if(!a.all_memory_deallocated() && a.check_sanity())
      return false;

   //Now clear all free memory
   a.zero_free_memory();

   if(!a.all_memory_deallocated() && a.check_sanity())
      return false;

   //Now test all allocated memory is zero
   //Allocate memory
   const char *first_addr = 0;
   for(int i = 0; true; ++i){
      void *ptr = a.allocate(i, std::nothrow);
      if(!ptr)
         break;
      if(i == 0){
         first_addr = (char*)ptr;
      }
      std::size_t memsize = a.size(ptr);
      buffers.push_back(ptr);

      for(int j = 0; j < (int)memsize; ++j){
         if(static_cast<char*>((char*)ptr)[j]){
            std::cout << "Zero memory test failed. in buffer " << i
                      << " byte " << j << " first address " << (void*) first_addr << " offset " << ((char*)ptr+j) - (char*)first_addr << " memsize: " << memsize << std::endl;
            return false;
         }
      }
   }

   //Deallocate all
   for(int j = (int)buffers.size()
      ;j--
      ;){
      a.deallocate(buffers[j]);
   }
   if(!a.all_memory_deallocated() && a.check_sanity())
      return false;

   return true;
}


//This test uses tests grow and shrink_to_fit functions
template<class Allocator>
bool test_grow_shrink_to_fit(Allocator &a)
{
   std::vector<void*> buffers;

   typename Allocator::size_type original_size = a.get_size();
   typename Allocator::size_type original_free = a.get_free_memory();

   a.shrink_to_fit();

   if(!a.all_memory_deallocated() && a.check_sanity())
      return false;

   typename Allocator::size_type shrunk_size          = a.get_size();
   typename Allocator::size_type shrunk_free_memory   = a.get_free_memory();
   if(shrunk_size != a.get_min_size())
      return 1;

   a.grow(original_size - shrunk_size);

   if(!a.all_memory_deallocated() && a.check_sanity())
      return false;

   if(original_size != a.get_size())
      return false;
   if(original_free != a.get_free_memory())
      return false;

   //Allocate memory
   for(int i = 0; true; ++i){
      void *ptr = a.allocate(i, std::nothrow);
      if(!ptr)
         break;
      std::size_t size = a.size(ptr);
      std::memset(ptr, 0, size);
      buffers.push_back(ptr);
   }

   //Now deallocate the half of the blocks
   //so expand maybe can merge new free blocks
   for(int i = 0, max = (int)buffers.size()
      ;i < max
      ;++i){
      if(i%2){
         a.deallocate(buffers[i]);
         buffers[i] = 0;
      }
   }

   //Deallocate the rest of the blocks

   //Deallocate it in non sequential order
   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      int pos = (j%5)*((int)buffers.size())/4;
      if(pos == int(buffers.size()))
         --pos;
      a.deallocate(buffers[pos]);
      buffers.erase(buffers.begin()+pos);
      typename Allocator::size_type old_free = a.get_free_memory();
      a.shrink_to_fit();
      if(!a.check_sanity())   return false;
      if(original_size < a.get_size())    return false;
      if(old_free < a.get_free_memory())  return false;

      a.grow(original_size - a.get_size());

      if(!a.check_sanity())   return false;
      if(original_size != a.get_size())         return false;
      if(old_free      != a.get_free_memory())  return false;
   }

   //Now shrink it to the maximum
   a.shrink_to_fit();

   if(a.get_size() != a.get_min_size())
      return 1;

   if(shrunk_free_memory != a.get_free_memory())
      return 1;

   if(!a.all_memory_deallocated() && a.check_sanity())
      return false;

   a.grow(original_size - shrunk_size);

   if(original_size != a.get_size())
      return false;
   if(original_free != a.get_free_memory())
      return false;

   if(!a.all_memory_deallocated() && a.check_sanity())
      return false;
   return true;
}

//This test allocates multiple values until there is no more memory
//and after that deallocates all in the inverse order
template<class Allocator>
bool test_many_equal_allocation(Allocator &a)
{
   for( deallocation_type t = DirectDeallocation
      ; t != EndDeallocationType
      ; t = (deallocation_type)((int)t + 1)){
      typename Allocator::size_type free_memory = a.get_free_memory();

      std::vector<void*> buffers2;

      //Allocate buffers with extra memory
      for(int i = 0; true; ++i){
         void *ptr = a.allocate(i, std::nothrow);
         if(!ptr)
            break;
		 std::size_t size = a.size(ptr);
         std::memset(ptr, 0, size);
         if(!a.check_sanity())
            return false;
         buffers2.push_back(ptr);
      }

      //Now deallocate the half of the blocks
      //so expand maybe can merge new free blocks
      for(int i = 0, max = (int)buffers2.size()
         ;i < max
         ;++i){
         if(i%2){
            a.deallocate(buffers2[i]);
            buffers2[i] = 0;
         }
      }

      if(!a.check_sanity())
         return false;

      typedef typename Allocator::multiallocation_chain multiallocation_chain;
      std::vector<void*> buffers;
      for(int i = 0; true; ++i){
         multiallocation_chain chain;
         a.allocate_many(std::nothrow, i+1, (i+1)*2, chain);
         if(chain.empty())
            break;

         typename multiallocation_chain::size_type n = chain.size();
         while(!chain.empty()){
            buffers.push_back(ipcdetail::to_raw_pointer(chain.pop_front()));
         }
         if(n != std::size_t((i+1)*2))
            return false;
      }

      if(!a.check_sanity())
         return false;

      switch(t){
         case DirectDeallocation:
         {
            for(int j = 0, max = (int)buffers.size()
               ;j < max
               ;++j){
               a.deallocate(buffers[j]);
            }
         }
         break;
         case InverseDeallocation:
         {
            for(int j = (int)buffers.size()
               ;j--
               ;){
               a.deallocate(buffers[j]);
            }
         }
         break;
         case MixedDeallocation:
         {
            for(int j = 0, max = (int)buffers.size()
               ;j < max
               ;++j){
               int pos = (j%4)*((int)buffers.size())/4;
               a.deallocate(buffers[pos]);
               buffers.erase(buffers.begin()+pos);
            }
         }
         break;
         default:
         break;
      }

      //Deallocate the rest of the blocks

      //Deallocate it in non sequential order
      for(int j = 0, max = (int)buffers2.size()
         ;j < max
         ;++j){
         int pos = (j%4)*((int)buffers2.size())/4;
         a.deallocate(buffers2[pos]);
         buffers2.erase(buffers2.begin()+pos);
      }

      bool ok = free_memory == a.get_free_memory() &&
               a.all_memory_deallocated() && a.check_sanity();
      if(!ok)  return ok;
   }
   return true;
}

//This test allocates multiple values until there is no more memory
//and after that deallocates all in the inverse order
template<class Allocator>
bool test_many_different_allocation(Allocator &a)
{
   typedef typename Allocator::multiallocation_chain multiallocation_chain;
   const std::size_t ArraySize = 11;
   typename Allocator::size_type requested_sizes[ArraySize];
   for(std::size_t i = 0; i < ArraySize; ++i){
      requested_sizes[i] = 4*i;
   }

   for( deallocation_type t = DirectDeallocation
      ; t != EndDeallocationType
      ; t = (deallocation_type)((int)t + 1)){
      typename Allocator::size_type free_memory = a.get_free_memory();

      std::vector<void*> buffers2;

      //Allocate buffers with extra memory
      for(int i = 0; true; ++i){
         void *ptr = a.allocate(i, std::nothrow);
         if(!ptr)
            break;
		 std::size_t size = a.size(ptr);
         std::memset(ptr, 0, size);
         buffers2.push_back(ptr);
      }

      //Now deallocate the half of the blocks
      //so expand maybe can merge new free blocks
      for(int i = 0, max = (int)buffers2.size()
         ;i < max
         ;++i){
         if(i%2){
            a.deallocate(buffers2[i]);
            buffers2[i] = 0;
         }
      }

      std::vector<void*> buffers;
      for(int i = 0; true; ++i){
         multiallocation_chain chain;
         a.allocate_many(std::nothrow, requested_sizes, ArraySize, 1, chain);
         if(chain.empty())
            break;
         typename multiallocation_chain::size_type n = chain.size();
         while(!chain.empty()){
            buffers.push_back(ipcdetail::to_raw_pointer(chain.pop_front()));
         }
         if(n != ArraySize)
            return false;
      }

      switch(t){
         case DirectDeallocation:
         {
            for(int j = 0, max = (int)buffers.size()
               ;j < max
               ;++j){
               a.deallocate(buffers[j]);
            }
         }
         break;
         case InverseDeallocation:
         {
            for(int j = (int)buffers.size()
               ;j--
               ;){
               a.deallocate(buffers[j]);
            }
         }
         break;
         case MixedDeallocation:
         {
            for(int j = 0, max = (int)buffers.size()
               ;j < max
               ;++j){
               int pos = (j%4)*((int)buffers.size())/4;
               a.deallocate(buffers[pos]);
               buffers.erase(buffers.begin()+pos);
            }
         }
         break;
         default:
         break;
      }

      //Deallocate the rest of the blocks

      //Deallocate it in non sequential order
      for(int j = 0, max = (int)buffers2.size()
         ;j < max
         ;++j){
         int pos = (j%4)*((int)buffers2.size())/4;
         a.deallocate(buffers2[pos]);
         buffers2.erase(buffers2.begin()+pos);
      }

      bool ok = free_memory == a.get_free_memory() &&
               a.all_memory_deallocated() && a.check_sanity();
      if(!ok)  return ok;
   }
   return true;
}

//This test allocates multiple values until there is no more memory
//and after that deallocates all in the inverse order
template<class Allocator>
bool test_many_deallocation(Allocator &a)
{
   typedef typename Allocator::multiallocation_chain multiallocation_chain;

   typedef typename Allocator::multiallocation_chain multiallocation_chain;
   const std::size_t ArraySize = 11;
   vector<multiallocation_chain> buffers;
   typename Allocator::size_type requested_sizes[ArraySize];
   for(std::size_t i = 0; i < ArraySize; ++i){
      requested_sizes[i] = 4*i;
   }
   typename Allocator::size_type free_memory = a.get_free_memory();

   {
      for(int i = 0; true; ++i){
         multiallocation_chain chain;
         a.allocate_many(std::nothrow, requested_sizes, ArraySize, 1, chain);
         if(chain.empty())
            break;
         buffers.push_back(boost::move(chain));
      }
      for(int i = 0, max = (int)buffers.size(); i != max; ++i){
         a.deallocate_many(buffers[i]);
      }
      buffers.clear();
      bool ok = free_memory == a.get_free_memory() &&
               a.all_memory_deallocated() && a.check_sanity();
      if(!ok)  return ok;
   }

   {
      for(int i = 0; true; ++i){
         multiallocation_chain chain;
         a.allocate_many(std::nothrow, i*4, ArraySize, chain);
         if(chain.empty())
            break;
         buffers.push_back(boost::move(chain));
      }
      for(int i = 0, max = (int)buffers.size(); i != max; ++i){
         a.deallocate_many(buffers[i]);
      }
      buffers.clear();

      bool ok = free_memory == a.get_free_memory() &&
               a.all_memory_deallocated() && a.check_sanity();
      if(!ok)  return ok;
   }

   return true;
}


//This function calls all tests
template<class Allocator>
bool test_all_allocation(Allocator &a)
{
   std::cout << "Starting test_allocation. Class: "
             << typeid(a).name() << std::endl;

   if(!test_allocation(a)){
      std::cout << "test_allocation_direct_deallocation failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_many_equal_allocation. Class: "
             << typeid(a).name() << std::endl;

   if(!test_many_equal_allocation(a)){
      std::cout << "test_many_equal_allocation failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_many_different_allocation. Class: "
             << typeid(a).name() << std::endl;

   if(!test_many_different_allocation(a)){
      std::cout << "test_many_different_allocation failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   if(!test_many_deallocation(a)){
      std::cout << "test_many_deallocation failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_allocation_shrink. Class: "
             << typeid(a).name() << std::endl;

   if(!test_allocation_shrink(a)){
      std::cout << "test_allocation_shrink failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   if(!test_allocation_shrink_and_expand(a)){
      std::cout << "test_allocation_shrink_and_expand failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_allocation_expand. Class: "
             << typeid(a).name() << std::endl;

   if(!test_allocation_expand(a)){
      std::cout << "test_allocation_expand failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_allocation_deallocation_expand. Class: "
             << typeid(a).name() << std::endl;

   if(!test_allocation_deallocation_expand(a)){
      std::cout << "test_allocation_deallocation_expand failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_allocation_with_reuse. Class: "
             << typeid(a).name() << std::endl;

   if(!test_allocation_with_reuse(a)){
      std::cout << "test_allocation_with_reuse failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_aligned_allocation. Class: "
             << typeid(a).name() << std::endl;

   if(!test_aligned_allocation(a)){
      std::cout << "test_aligned_allocation failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_continuous_aligned_allocation. Class: "
             << typeid(a).name() << std::endl;

   if(!test_continuous_aligned_allocation(a)){
      std::cout << "test_continuous_aligned_allocation failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_clear_free_memory. Class: "
             << typeid(a).name() << std::endl;

   if(!test_clear_free_memory(a)){
      std::cout << "test_clear_free_memory failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_grow_shrink_to_fit. Class: "
             << typeid(a).name() << std::endl;

   if(!test_grow_shrink_to_fit(a)){
      std::cout << "test_grow_shrink_to_fit failed. Class: "
                << typeid(a).name() << std::endl;
      return false;
   }

   return true;
}

}}}   //namespace boost { namespace interprocess { namespace test {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_TEST_MEMORY_ALGORITHM_TEST_TEMPLATE_HEADER

