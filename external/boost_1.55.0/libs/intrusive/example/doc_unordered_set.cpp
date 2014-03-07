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
//[doc_unordered_set_code
#include <boost/intrusive/unordered_set.hpp>
#include <vector>
#include <algorithm>
#include <boost/functional/hash.hpp>

using namespace boost::intrusive;

class MyClass : public unordered_set_base_hook<>
{               //This is a derivation hook
   int int_;

   public:
   unordered_set_member_hook<> member_hook_; //This is a member hook

   MyClass(int i)
      :  int_(i)
   {}

   friend bool operator== (const MyClass &a, const MyClass &b)
   {  return a.int_ == b.int_;  }

   friend std::size_t hash_value(const MyClass &value)
   {  return std::size_t(value.int_); }
};

//Define an unordered_set that will store MyClass objects using the base hook
typedef unordered_set<MyClass>    BaseSet;

//Define an unordered_multiset that will store MyClass using the member hook
typedef member_hook<MyClass, unordered_set_member_hook<>, &MyClass::member_hook_>
   MemberOption;
typedef unordered_multiset< MyClass, MemberOption>  MemberMultiSet;

int main()
{
   typedef std::vector<MyClass>::iterator VectIt;

   //Create a vector with 100 different MyClass objects
   std::vector<MyClass> values;
   for(int i = 0; i < 100; ++i)  values.push_back(MyClass(i));

   //Create a copy of the vector
   std::vector<MyClass> values2(values);

   //Create a bucket array for base_set
   BaseSet::bucket_type base_buckets[100];

   //Create a bucket array for member_multi_set
   MemberMultiSet::bucket_type member_buckets[200];

   //Create unordered containers taking buckets as arguments
   BaseSet base_set(BaseSet::bucket_traits(base_buckets, 100));
   MemberMultiSet member_multi_set
      (MemberMultiSet::bucket_traits(member_buckets, 200));

   //Now insert values's elements in the unordered_set
   for(VectIt it(values.begin()), itend(values.end()); it != itend; ++it)
      base_set.insert(*it);

   //Now insert values's and values2's elements in the unordered_multiset
   for(VectIt it(values.begin()), itend(values.end()), it2(values2.begin())
      ; it != itend; ++it, ++it2){
      member_multi_set.insert(*it);
      member_multi_set.insert(*it2);
   }

   //Now find every element
   {
      VectIt it(values.begin()), itend(values.end());

      for(; it != itend; ++it){
         //base_set should contain one element for each key
         if(base_set.count(*it) != 1)           return 1;
         //member_multi_set should contain two elements for each key
         if(member_multi_set.count(*it) != 2)   return 1;
      }
   }
   return 0;
}
//]
