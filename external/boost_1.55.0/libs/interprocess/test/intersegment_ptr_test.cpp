//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2007-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/intersegment_ptr.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/mapped_region.hpp> //mapped_region
#include <boost/interprocess/anonymous_shared_memory.hpp>   //anonymous_shared_memory
#include <boost/interprocess/detail/managed_multi_shared_memory.hpp>   //managed_multi_shared_memory
#include <boost/static_assert.hpp>   //static_assert
#include <cstddef>   //std::size_t


using namespace boost::interprocess;

bool test_types_and_convertions()
{
   typedef intersegment_ptr<int>                pint_t;
   typedef intersegment_ptr<const int>          pcint_t;
   typedef intersegment_ptr<volatile int>       pvint_t;
   typedef intersegment_ptr<const volatile int> pcvint_t;

   BOOST_STATIC_ASSERT((ipcdetail::is_same<pint_t::value_type, int>::value));
   BOOST_STATIC_ASSERT((ipcdetail::is_same<pcint_t::value_type, const int>::value));
   BOOST_STATIC_ASSERT((ipcdetail::is_same<pvint_t::value_type, volatile int>::value));
   BOOST_STATIC_ASSERT((ipcdetail::is_same<pcvint_t::value_type, const volatile int>::value));
   int dummy_int = 9;

   {  pint_t pint(&dummy_int);   pcint_t  pcint(pint);
      if(pcint.get()  != &dummy_int)   return false;  }
   {  pint_t pint(&dummy_int);   pvint_t  pvint(pint);
      if(pvint.get()  != &dummy_int)   return false;  }
   {  pint_t pint(&dummy_int);   pcvint_t  pcvint(pint);
      if(pcvint.get()  != &dummy_int)  return false;  }
   {  pcint_t pcint(&dummy_int); pcvint_t  pcvint(pcint);
         if(pcvint.get()  != &dummy_int)  return false;  }
   {  pvint_t pvint(&dummy_int); pcvint_t  pcvint(pvint);
      if(pcvint.get()  != &dummy_int)  return false;  }

   pint_t   pint(0);
   pcint_t  pcint(0);
   pvint_t  pvint(0);
   pcvint_t pcvint(0);

   pint     = &dummy_int;
   pcint    = &dummy_int;
   pvint    = &dummy_int;
   pcvint   = &dummy_int;

   {   pcint  = pint;   if(pcint.get() != &dummy_int)   return false;  }
   {   pvint  = pint;   if(pvint.get() != &dummy_int)   return false;  }
   {   pcvint = pint;   if(pcvint.get() != &dummy_int)  return false;  }
   {   pcvint = pcint;  if(pcvint.get() != &dummy_int)  return false;  }
   {   pcvint = pvint;  if(pcvint.get() != &dummy_int)  return false;  }

   if(!pint)
      return false;

   pint = 0;
   if(pint)
      return false;

   return true;
}

bool test_arithmetic()
{
   typedef intersegment_ptr<int> pint_t;
   const int NumValues = 5;
   int values[NumValues];

   //Initialize p
   pint_t p = values;
   if(p.get() != values)
      return false;

   //Initialize p + NumValues
   pint_t pe = &values[NumValues];
   if(pe == p)
      return false;
   if(pe.get() != &values[NumValues])
      return false;

   //ptr - ptr
   if((pe - p) != NumValues)
      return false;
   //ptr - integer
   if((pe - NumValues) != p)
      return false;
   //ptr + integer
   if((p + NumValues) != pe)
      return false;
   //integer + ptr
   if((NumValues + p) != pe)
      return false;
   //indexing
   if(pint_t(&p[NumValues])   != pe)
      return false;
   if(pint_t(&pe[-NumValues]) != p)
      return false;

   //ptr -= integer
   pint_t p0 = pe;
   p0-= NumValues;
   if(p != p0)
      return false;
   //ptr += integer
   pint_t penew = p0;
   penew += NumValues;
   if(penew != pe)
      return false;

   //++ptr
   penew = p0;
   for(int j = 0; j != NumValues; ++j, ++penew);
   if(penew != pe)
      return false;
   //--ptr
   p0 = pe;
   for(int j = 0; j != NumValues; ++j, --p0);
   if(p != p0)
      return false;
   //ptr++
   penew = p0;
   for(int j = 0; j != NumValues; ++j){
      pint_t pnew_copy = penew;
      if(pnew_copy != penew++)
         return false;
   }
   //ptr--
   p0 = pe;
   for(int j = 0; j != NumValues; ++j){
      pint_t p0_copy = p0;
      if(p0_copy != p0--)
         return false;
   }

   return true;
}

bool test_comparison()
{
   typedef intersegment_ptr<int> pint_t;
   const int NumValues = 5;
   int values[NumValues];

   //Initialize p
   pint_t p = values;
   if(p.get() != values)
      return false;

   //Initialize p + NumValues
   pint_t pe = &values[NumValues];
   if(pe == p)
      return false;
   if(pe.get() != &values[NumValues])
      return false;

   //operators
   if(p == pe)
      return false;
   if(p != p)
      return false;
   if(!(p < pe))
      return false;
   if(!(p <= pe))
      return false;
   if(!(pe > p))
      return false;
   if(!(pe >= p))
      return false;

   return true;
}

struct segment_data
{
   int int0;
   int int1;
   intersegment_ptr<int> ptr0;
   int int2;
   int int3;
};

bool test_basic_comparisons()
{
   //Create aligned sections
   const std::size_t PageSize = mapped_region::get_page_size();
   mapped_region reg_0_0(anonymous_shared_memory(PageSize));
   mapped_region reg_0_1(anonymous_shared_memory(PageSize));
   mapped_region reg_1_0(anonymous_shared_memory(PageSize));
   mapped_region reg_1_1(anonymous_shared_memory(PageSize));

   if(sizeof(segment_data) > mapped_region::get_page_size())
      return false;

   segment_data &seg_0_0 = *static_cast<segment_data *>(reg_0_0.get_address());
   segment_data &seg_0_1 = *static_cast<segment_data *>(reg_0_1.get_address());
   segment_data &seg_1_0 = *static_cast<segment_data *>(reg_1_0.get_address());
   segment_data &seg_1_1 = *static_cast<segment_data *>(reg_1_1.get_address());

   //Some dummy multi_segment_services
   multi_segment_services *services0 = static_cast<multi_segment_services *>(0);
   multi_segment_services *services1 = reinterpret_cast<multi_segment_services *>(1);

   const intersegment_ptr<int>::segment_group_id group_0_id =
      intersegment_ptr<int>::new_segment_group(services0);
   const intersegment_ptr<int>::segment_group_id group_1_id =
      intersegment_ptr<int>::new_segment_group(services1);

   {

      //Now register the segments in the segment data-base
      intersegment_ptr<int>::insert_mapping(group_0_id, &seg_0_0, PageSize);
      intersegment_ptr<int>::insert_mapping(group_0_id, &seg_0_1, PageSize);
      intersegment_ptr<int>::insert_mapping(group_1_id, &seg_1_0, PageSize);
      intersegment_ptr<int>::insert_mapping(group_1_id, &seg_1_1, PageSize);
   }

   //Now do some testing...
   {
      //Same segment
      seg_0_0.ptr0 = &seg_0_0.int0;
      seg_0_1.ptr0 = &seg_0_1.int0;

      if(seg_0_0.ptr0.get() != &seg_0_0.int0)
         return false;
      if(seg_0_1.ptr0.get() != &seg_0_1.int0)
         return false;

      //Try it again to make use of the already established relative addressing
      seg_0_0.ptr0 = &seg_0_0.int1;
      seg_0_1.ptr0 = &seg_0_1.int1;

      if(seg_0_0.ptr0.get() != &seg_0_0.int1)
         return false;
      if(seg_0_1.ptr0.get() != &seg_0_1.int1)
         return false;

      //Set to null and try again
      seg_0_0.ptr0 = 0;
      seg_0_1.ptr0 = 0;

      seg_0_0.ptr0 = &seg_0_0.int1;
      seg_0_1.ptr0 = &seg_0_1.int1;

      if(seg_0_0.ptr0.get() != &seg_0_0.int1)
         return false;
      if(seg_0_1.ptr0.get() != &seg_0_1.int1)
         return false;

      //Set to null and try again
      int stack_int;
      seg_0_0.ptr0 = &stack_int;
      seg_0_1.ptr0 = &stack_int;

      if(seg_0_0.ptr0.get() != &stack_int)
         return false;
      if(seg_0_1.ptr0.get() != &stack_int)
         return false;
   }

   {
      //Now use stack variables
      intersegment_ptr<int> stack_0 = &seg_0_0.int2;
      intersegment_ptr<int> stack_1 = &seg_1_1.int2;

      if(stack_0.get() != &seg_0_0.int2)
         return false;
      if(stack_1.get() != &seg_1_1.int2)
         return false;

      //Now reuse stack variables knowing that there are on stack
      stack_0 = &seg_0_0.int3;
      stack_1 = &seg_1_1.int3;

      if(stack_0.get() != &seg_0_0.int3)
         return false;
      if(stack_1.get() != &seg_1_1.int3)
         return false;

      //Now set to null and try it again
      stack_0 = 0;
      stack_1 = 0;

      stack_0 = &seg_0_0.int3;
      stack_1 = &seg_1_1.int3;

      if(stack_0.get() != &seg_0_0.int3)
         return false;
      if(stack_1.get() != &seg_1_1.int3)
         return false;
   }
   {
      //Different segments in the same group
      seg_0_0.ptr0 = &seg_0_1.int0;
      seg_0_1.ptr0 = &seg_0_0.int0;

      if(seg_0_0.ptr0.get() != &seg_0_1.int0)
         return false;
      if(seg_0_1.ptr0.get() != &seg_0_0.int0)
         return false;

      //Try it again to make use of the already established segmented addressing
      seg_0_0.ptr0 = &seg_0_1.int1;
      seg_0_1.ptr0 = &seg_0_0.int1;

      if(seg_0_0.ptr0.get() != &seg_0_1.int1)
         return false;
      if(seg_0_1.ptr0.get() != &seg_0_0.int1)
         return false;

      //Set to null and try it again
      seg_0_0.ptr0 = 0;
      seg_0_1.ptr0 = 0;

      seg_0_0.ptr0 = &seg_0_1.int1;
      seg_0_1.ptr0 = &seg_0_0.int1;

      if(seg_0_0.ptr0.get() != &seg_0_1.int1)
         return false;
      if(seg_0_1.ptr0.get() != &seg_0_0.int1)
         return false;
   }

   {
      //Different groups
      seg_0_0.ptr0 = &seg_1_0.int0;
      seg_0_1.ptr0 = &seg_1_1.int0;

      if(seg_0_0.ptr0.get() != &seg_1_0.int0)
         return false;
      if(seg_0_1.ptr0.get() != &seg_1_1.int0)
         return false;

      //Try it again
      seg_0_0.ptr0 = &seg_1_0.int1;
      seg_0_1.ptr0 = &seg_1_1.int1;

      if(seg_0_0.ptr0.get() != &seg_1_0.int1)
         return false;
      if(seg_0_1.ptr0.get() != &seg_1_1.int1)
         return false;

      //Set null and try it again
      seg_0_0.ptr0 = 0;
      seg_0_1.ptr0 = 0;

      seg_0_0.ptr0 = &seg_1_0.int1;
      seg_0_1.ptr0 = &seg_1_1.int1;

      if(seg_0_0.ptr0.get() != &seg_1_0.int1)
         return false;
      if(seg_0_1.ptr0.get() != &seg_1_1.int1)
         return false;
   }

   {
      //Erase mappings
      intersegment_ptr<int>::delete_group(group_0_id);
      intersegment_ptr<int>::delete_group(group_1_id);
   }
   return true;
}

bool test_multi_segment_shared_memory()
{
   {
      shared_memory_object::remove("kk0");
      managed_multi_shared_memory mshm(create_only, "kk", 4096);
   }

   shared_memory_object::remove("kk0");
   return true;
}

int main()
{/*
   if(!test_types_and_convertions())
      return 1;
   if(!test_arithmetic())
      return 1;
   if(!test_comparison())
      return 1;
   if(!test_basic_comparisons())
      return 1;

   if(!test_multi_segment_shared_memory())
      return 1;
*/
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
