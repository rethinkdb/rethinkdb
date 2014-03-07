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
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/smart_ptr/deleter.hpp>
#include <stdio.h>
#include <string>
#include "get_process_id_name.hpp"

using namespace boost::interprocess;

class MyClass
{
   public:
   MyClass()
   {}
};

typedef managed_unique_ptr<MyClass, managed_shared_memory>::type my_unique_ptr_class;
typedef set <my_unique_ptr_class
            ,std::less<my_unique_ptr_class>
            ,allocator  <my_unique_ptr_class
                        ,managed_shared_memory::segment_manager>
            > MySet;

typedef list<my_unique_ptr_class
            ,allocator  <my_unique_ptr_class
                        ,managed_shared_memory::segment_manager>
            > MyList;

typedef vector <my_unique_ptr_class
               ,allocator  <my_unique_ptr_class
                           ,managed_shared_memory::segment_manager>
            > MyVector;

int main()
{
   std::string process_name;
   test::get_process_id_name(process_name);

   //Create managed shared memory
   shared_memory_object::remove(process_name.c_str());
   {
      managed_shared_memory segment(create_only, process_name.c_str(), 10000);

      //Create unique_ptr using dynamic allocation
      my_unique_ptr_class my_ptr (segment.construct<MyClass>(anonymous_instance)()
                                 ,segment.get_deleter<MyClass>());
      my_unique_ptr_class my_ptr2(segment.construct<MyClass>(anonymous_instance)()
                                 ,segment.get_deleter<MyClass>());

      //Backup relative pointers to future tests
      offset_ptr<MyClass> ptr1 = my_ptr.get();
      offset_ptr<MyClass> ptr2 = my_ptr2.get();

      //Test some copy constructors
      my_unique_ptr_class my_ptr3(0, segment.get_deleter<MyClass>());
      my_unique_ptr_class my_ptr4(boost::move(my_ptr3));

      //Construct a list and fill
      MyList list(segment.get_segment_manager());

      //Insert from my_unique_ptr_class
      list.push_front(boost::move(my_ptr));
      list.push_back(boost::move(my_ptr2));

      //Check pointers
      assert(my_ptr.get() == 0);
      assert(my_ptr2.get() == 0);
      assert(list.begin()->get() == ptr1);
      assert(list.rbegin()->get() == ptr2);

      //Construct a set and fill
      typedef std::less<my_unique_ptr_class> set_less_t;
      MySet set(set_less_t(), segment.get_segment_manager());

      //Insert in set from list passing ownership
      set.insert(boost::move(*list.begin()));
      set.insert(boost::move(*list.rbegin()));

      //Check pointers
      assert(list.begin()->get() == 0);
      assert(list.rbegin()->get()== 0);

      //A set is ordered by std::less<my_unique_ptr_class> so
      //be careful when comparing pointers
      if(ptr1 < ptr2){
         assert(set.begin()->get()  == ptr1);
         assert(set.rbegin()->get() == ptr2);
      }
      else{
         assert(set.rbegin()->get() == ptr1);
         assert(set.begin()->get()  == ptr2);
      }

      //Now with vector
      MyVector vector(segment.get_segment_manager());

      //Insert from my_unique_ptr_class
      if(ptr1 < ptr2){
         vector.insert(vector.begin(), boost::move(*set.begin()));
         vector.insert(vector.end(),   boost::move(*set.rbegin()));
      }
      else{
         vector.insert(vector.begin(), boost::move(*set.rbegin()));
         vector.insert(vector.end(),   boost::move(*set.begin()));
      }

      //Check pointers
      assert(my_ptr.get() == 0);
      assert(my_ptr2.get() == 0);
      assert(vector.begin()->get() == ptr1);
      assert(vector.rbegin()->get() == ptr2);

      MyVector vector2(boost::move(vector));
      vector2.swap(vector);

      assert(vector.begin()->get() == ptr1);
      assert(vector.rbegin()->get() == ptr2);

      my_unique_ptr_class a(0, segment.get_deleter<MyClass>()), b(0, segment.get_deleter<MyClass>());
      a = boost::move(b);
   }
   shared_memory_object::remove(process_name.c_str());
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
