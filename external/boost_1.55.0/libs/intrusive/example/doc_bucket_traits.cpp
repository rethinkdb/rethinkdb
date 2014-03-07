/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2007-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
//[doc_bucket_traits
#include <boost/intrusive/unordered_set.hpp>
#include <boost/functional/hash.hpp>
#include <vector>

using namespace boost::intrusive;

//A class to be inserted in an unordered_set
class MyClass : public unordered_set_base_hook<>
{
   int int_;

   public:
   MyClass(int i = 0) : int_(i)
   {}

   friend bool operator==(const MyClass &l, const MyClass &r)
      {  return l.int_ == r.int_;   }
   friend std::size_t hash_value(const MyClass &v)
      {  return boost::hash_value(v.int_); }
};

//Define the base hook option
typedef base_hook< unordered_set_base_hook<> >     BaseHookOption;

//Obtain the types of the bucket and the bucket pointer
typedef unordered_bucket<BaseHookOption>::type     BucketType;
typedef unordered_bucket_ptr<BaseHookOption>::type BucketPtr;

//The custom bucket traits.
class custom_bucket_traits
{
   public:
   static const int NumBuckets = 100;

   custom_bucket_traits(BucketPtr buckets)
      :  buckets_(buckets)
   {}

   //Functions to be implemented by custom bucket traits
   BucketPtr   bucket_begin() const {  return buckets_;  }
   std::size_t bucket_count() const {  return NumBuckets;}

   private:
   BucketPtr buckets_;
};

//Define the container using the custom bucket traits
typedef unordered_set<MyClass, bucket_traits<custom_bucket_traits> > BucketTraitsUset;

int main()
{
   typedef std::vector<MyClass>::iterator VectIt;
   std::vector<MyClass> values;

   //Fill values
   for(int i = 0; i < 100; ++i)  values.push_back(MyClass(i));

   //Now create the bucket array and the custom bucket traits object
   BucketType buckets[custom_bucket_traits::NumBuckets];
   custom_bucket_traits btraits(buckets);

   //Now create the unordered set
   BucketTraitsUset uset(btraits);

   //Insert the values in the unordered set
   for(VectIt it(values.begin()), itend(values.end()); it != itend; ++it)
      uset.insert(*it);

   return 0;
}
//]
