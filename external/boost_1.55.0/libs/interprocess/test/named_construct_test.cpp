//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2008-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <utility>

typedef std::pair<double, int> simple_pair;

using namespace boost::interprocess;

struct array_pair :  public simple_pair
{
   array_pair(double d, int i)
      :  simple_pair(d, i) {}
};

struct array_it_pair :  public array_pair
{
   array_it_pair(double d, int i)
      :  array_pair(d, i)  {}
};

struct named_name_generator
{
   static const bool searchable = true;

   typedef simple_pair     simple_type;
   typedef array_pair      array_type;
   typedef array_it_pair   array_it_type;
   static const char *get_simple_name()
   {  return "MyType instance";  }
   static const char *get_array_name()
   {  return "MyType array";  }
   static const char *get_array_it_name()
   {  return "MyType array from it";   }
};

struct unique_name_generator
{
   static const bool searchable = true;

   typedef simple_pair     simple_type;
   typedef array_pair      array_type;
   typedef array_it_pair   array_it_type;
   static const ipcdetail::unique_instance_t *get_simple_name()
   {  return 0;  }
   static const ipcdetail::unique_instance_t *get_array_name()
   {  return 0;  }
   static const ipcdetail::unique_instance_t *get_array_it_name()
   {  return 0;  }
};

struct anonymous_name_generator
{
   static const bool searchable = false;

   typedef simple_pair simple_type;
   typedef array_pair array_type;
   typedef array_it_pair array_it_type;
   static const ipcdetail::anonymous_instance_t *get_simple_name()
   {  return 0;  }
   static const ipcdetail::anonymous_instance_t *get_array_name()
   {  return 0;  }
   static const ipcdetail::anonymous_instance_t *get_array_it_name()
   {  return 0;  }
};


template<class NameGenerator>
int construct_test()
{
   typedef typename NameGenerator::simple_type     simple_type;
   typedef typename NameGenerator::array_type      array_type;
   typedef typename NameGenerator::array_it_type   array_it_type;

   remove_shared_memory_on_destroy remover("MySharedMemory");
   shared_memory_object::remove("MySharedMemory");
   {
      //A special shared memory where we can
      //construct objects associated with a name.
      //First remove any old shared memory of the same name, create
      //the shared memory segment and initialize needed resources
      managed_shared_memory segment
         //create       segment name    segment size
         (create_only, "MySharedMemory", 65536);

      //Create an object of MyType initialized to {0.0, 0}
      simple_type *s = segment.construct<simple_type>
         (NameGenerator::get_simple_name())//name of the object
         (1.0, 2);            //ctor first argument
      assert(s->first == 1.0 && s->second == 2);
      if(!(s->first == 1.0 && s->second == 2))
         return 1;

      //Create an array of 10 elements of MyType initialized to {0.0, 0}
      array_type *a = segment.construct<array_type>
         (NameGenerator::get_array_name()) //name of the object
         [10]                 //number of elements
         (3.0, 4);            //Same two ctor arguments for all objects
      assert(a->first == 3.0 && a->second == 4);
      if(!(a->first == 3.0 && a->second == 4))
         return 1;

      //Create an array of 3 elements of MyType initializing each one
      //to a different value {0.0, 3}, {1.0, 4}, {2.0, 5}...
      float float_initializer[3] = { 0.0, 1.0, 2.0 };
      int   int_initializer[3]   = { 3, 4, 5 };

      array_it_type *a_it = segment.construct_it<array_it_type>
         (NameGenerator::get_array_it_name()) //name of the object
         [3]                        //number of elements
         ( &float_initializer[0]    //Iterator for the 1st ctor argument
         , &int_initializer[0]);    //Iterator for the 2nd ctor argument
      {
         const array_it_type *a_it_ptr = a_it;
         for(unsigned int i = 0, max = 3; i != max; ++i, ++a_it_ptr){
            assert(a_it_ptr->first == float_initializer[i]);
            if(a_it_ptr->first != float_initializer[i]){
               return 1;
            }
            assert(a_it_ptr->second == int_initializer[i]);
            if(a_it_ptr->second != int_initializer[i]){
               return 1;
            }
         }
      }

      if(NameGenerator::searchable){
         {
            std::pair<simple_type*, managed_shared_memory::size_type> res;
            //Find the object
            res = segment.find<simple_type> (NameGenerator::get_simple_name());
            //Length should be 1
            assert(res.second == 1);
            if(res.second != 1)
               return 1;
            assert(res.first == s);
            if(res.first != s)
               return 1;
         }
         {
            std::pair<array_type*, managed_shared_memory::size_type> res;

            //Find the array
            res = segment.find<array_type> (NameGenerator::get_array_name());
            //Length should be 10
            assert(res.second == 10);
            if(res.second != 10)
               return 1;
            assert(res.first == a);
            if(res.first != a)
               return 1;
         }
         {
            std::pair<array_it_type*, managed_shared_memory::size_type> res;
            //Find the array constructed from iterators
            res = segment.find<array_it_type> (NameGenerator::get_array_it_name());
            //Length should be 3
            assert(res.second == 3);
            if(res.second != 3)
               return 1;
            assert(res.first == a_it);
            if(res.first != a_it)
               return 1;
         }
      }
      //We're done, delete all the objects
      segment.destroy_ptr<simple_type>(s);
      segment.destroy_ptr<array_type>(a);
      segment.destroy_ptr<array_it_type>(a_it);
   }
   return 0;
}

int main ()
{
   if(0 != construct_test<named_name_generator>())
      return 1;
   if(0 != construct_test<unique_name_generator>())
      return 1;
   if(0 != construct_test<anonymous_name_generator>())
      return 1;
   return 0;
}

//]
#include <boost/interprocess/detail/config_end.hpp>
