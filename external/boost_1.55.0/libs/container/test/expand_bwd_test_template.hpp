//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINER_TEST_ALLOCATION_TEST_TEMPLATE_HEADER
#define BOOST_CONTAINER_TEST_ALLOCATION_TEST_TEMPLATE_HEADER

#include <boost/container/detail/config_begin.hpp>
#include <vector>
#include <typeinfo>
#include "expand_bwd_test_allocator.hpp"
#include <algorithm>
#include <boost/type_traits/remove_volatile.hpp>

namespace boost { namespace container { namespace test {

template<class T>
struct value_holder
{
   value_holder(T val)  :  m_value(val){}
   value_holder(): m_value(0){}
   ~value_holder(){ m_value = 0; }
   bool operator == (const value_holder &other) const
   {  return m_value == other.m_value; }
   bool operator != (const value_holder &other) const
   {  return m_value != other.m_value; }

   T m_value;
};

template<class T>
struct triple_value_holder
{
   triple_value_holder(T val)
      :  m_value1(val)
      ,  m_value2(val)
      ,  m_value3(val)
   {}

   triple_value_holder()
      :  m_value1(0)
      ,  m_value2(0)
      ,  m_value3(0)
   {}

   ~triple_value_holder()
   {  m_value1 = m_value2 = m_value3 = 0; }

   bool operator == (const triple_value_holder &other) const
   {
      return   m_value1 == other.m_value1
         &&    m_value2 == other.m_value2
         &&    m_value3 == other.m_value3;
   }

   bool operator != (const triple_value_holder &other) const
   {
      return   m_value1 != other.m_value1
         ||    m_value2 != other.m_value2
         ||    m_value3 != other.m_value3;
   }

   T m_value1;
   T m_value2;
   T m_value3;
};

typedef value_holder<int> int_holder;
typedef triple_value_holder<int> triple_int_holder;



//Function to check if both sets are equal
template <class Vector1, class Vector2>
bool CheckEqualVector(const Vector1 &vector1, const Vector2 &vector2)
{
   if(vector1.size() != vector2.size())
      return false;
   return std::equal(vector1.begin(), vector1.end(), vector2.begin());
}

template<class Vector>
bool CheckUninitializedIsZero(const Vector & v)
{
   typedef  typename Vector::value_type value_type;
   typename Vector::size_type sz    = v.size();
   typename Vector::size_type extra = v.capacity() - v.size();
   value_type comp(0);

   const value_type *holder = &v[0] + sz;

   while(extra--){
      if(*holder++ != comp)
         return false;
   }
   return true;
}


//This function tests all the possible combinations when
//inserting data in a vector and expanding backwards
template<class VectorWithExpandBwdAllocator>
bool test_insert_with_expand_bwd()
{
   typedef typename VectorWithExpandBwdAllocator::value_type value_type;
   typedef typename boost::remove_volatile<value_type>::type non_volatile_value_type;
   typedef std::vector<non_volatile_value_type> Vect;
   const int MemorySize = 1000;

   //Distance old and new buffer
   const int Offset[]      =
      {  350,  250,  150,  150,
         150,  50,   50,   50    };
   //Insert position
   const int Position[]    =
      {  100,  100,  100,  100,
         100,  100,  100,  100   };
   //Initial vector size
   const int InitialSize[] =
      {  200,  200,  200,  200,
         200,  200,  200,  200   };
   //Size of the data to insert
   const int InsertSize[]  =
      {  100,  100,  100,  200,
         300,  25,   100,  200   };
   //Number of tests
   const int Iterations    = sizeof(InsertSize)/sizeof(int);

   for(int iteration = 0; iteration < Iterations; ++iteration)
   {
      value_type *memory = new value_type[MemorySize];
      BOOST_TRY {
         std::vector<non_volatile_value_type> initial_data;
         initial_data.resize(InitialSize[iteration]);
         for(int i = 0; i < InitialSize[iteration]; ++i){
            initial_data[i] = i;
         }

         Vect data_to_insert;
         data_to_insert.resize(InsertSize[iteration]);
         for(int i = 0; i < InsertSize[iteration]; ++i){
            data_to_insert[i] = -i;
         }

         expand_bwd_test_allocator<value_type> alloc
            (&memory[0], MemorySize, Offset[iteration]);
         VectorWithExpandBwdAllocator vector(alloc);
         vector.insert( vector.begin()
                     , initial_data.begin(), initial_data.end());
         vector.insert( vector.begin() + Position[iteration]
                     , data_to_insert.begin(), data_to_insert.end());
         initial_data.insert(initial_data.begin() + Position[iteration]
                           , data_to_insert.begin(), data_to_insert.end());
         //Now check that values are equal
         if(!CheckEqualVector(vector, initial_data)){
            std::cout << "test_assign_with_expand_bwd::CheckEqualVector failed." << std::endl
                     << "   Class: " << typeid(VectorWithExpandBwdAllocator).name() << std::endl
                     << "   Iteration: " << iteration << std::endl;
            return false;
         }
      }
      BOOST_CATCH(...){
         delete [](const_cast<non_volatile_value_type*>(memory));
         BOOST_RETHROW;
      }
      BOOST_CATCH_END
      delete [](const_cast<non_volatile_value_type*>(memory));
   }

   return true;
}

//This function tests all the possible combinations when
//inserting data in a vector and expanding backwards
template<class VectorWithExpandBwdAllocator>
bool test_assign_with_expand_bwd()
{
   typedef typename VectorWithExpandBwdAllocator::value_type value_type;
   typedef typename boost::remove_volatile<value_type>::type non_volatile_value_type;
   const int MemorySize = 200;

   const int Offset[]      = { 50, 50, 50};
   const int InitialSize[] = { 25, 25, 25};
   const int AssignSize[]  = { 40, 60, 80};
   const int Iterations    = sizeof(AssignSize)/sizeof(int);

   for(int iteration = 0; iteration <Iterations; ++iteration)
   {
      value_type *memory = new value_type[MemorySize];
      BOOST_TRY {
         //Create initial data
         std::vector<non_volatile_value_type> initial_data;
         initial_data.resize(InitialSize[iteration]);
         for(int i = 0; i < InitialSize[iteration]; ++i){
            initial_data[i] = i;
         }

         //Create data to assign
         std::vector<non_volatile_value_type> data_to_assign;
         data_to_assign.resize(AssignSize[iteration]);
         for(int i = 0; i < AssignSize[iteration]; ++i){
            data_to_assign[i] = -i;
         }

         //Insert initial data to the vector to test
         expand_bwd_test_allocator<value_type> alloc
            (&memory[0], MemorySize, Offset[iteration]);
         VectorWithExpandBwdAllocator vector(alloc);
         vector.insert( vector.begin()
                     , initial_data.begin(), initial_data.end());

         //Assign data
         vector.assign(data_to_assign.begin(), data_to_assign.end());
         initial_data.assign(data_to_assign.begin(), data_to_assign.end());

         //Now check that values are equal
         if(!CheckEqualVector(vector, initial_data)){
            std::cout << "test_assign_with_expand_bwd::CheckEqualVector failed." << std::endl
                     << "   Class: " << typeid(VectorWithExpandBwdAllocator).name() << std::endl
                     << "   Iteration: " << iteration << std::endl;
            return false;
         }
      }
      BOOST_CATCH(...){
         delete [](const_cast<typename boost::remove_volatile<value_type>::type*>(memory));
         BOOST_RETHROW;
      }
      BOOST_CATCH_END
      delete [](const_cast<typename boost::remove_volatile<value_type>::type*>(memory));
   }

   return true;
}

//This function calls all tests
template<class VectorWithExpandBwdAllocator>
bool test_all_expand_bwd()
{
   std::cout << "Starting test_insert_with_expand_bwd." << std::endl << "  Class: "
             << typeid(VectorWithExpandBwdAllocator).name() << std::endl;

   if(!test_insert_with_expand_bwd<VectorWithExpandBwdAllocator>()){
      std::cout << "test_allocation_direct_deallocation failed. Class: "
                << typeid(VectorWithExpandBwdAllocator).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_assign_with_expand_bwd." << std::endl << "  Class: "
             << typeid(VectorWithExpandBwdAllocator).name() << std::endl;

   if(!test_assign_with_expand_bwd<VectorWithExpandBwdAllocator>()){
      std::cout << "test_allocation_direct_deallocation failed. Class: "
                << typeid(VectorWithExpandBwdAllocator).name() << std::endl;
      return false;
   }

   return true;
}

}}}   //namespace boost { namespace container { namespace test {

#include <boost/container/detail/config_end.hpp>

#endif   //BOOST_CONTAINER_TEST_ALLOCATION_TEST_TEMPLATE_HEADER
