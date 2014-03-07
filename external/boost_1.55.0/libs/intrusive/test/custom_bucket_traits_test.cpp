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
#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive/detail/mpl.hpp>
#include <boost/functional/hash.hpp>
#include <vector>

using namespace boost::intrusive;

class MyClass : public unordered_set_base_hook<>
{
   int int_;

   public:
   MyClass(int i = 0) : int_(i)
   {}
   unordered_set_member_hook<> member_hook_;

   friend bool operator==(const MyClass &l, const MyClass &r)
   {  return l.int_ == r.int_;   }

   friend std::size_t hash_value(const MyClass &v)
   {  return boost::hash_value(v.int_); }
};

struct uset_value_traits
{
   typedef slist_node_traits<void*>          node_traits;
   typedef node_traits::node_ptr             node_ptr;
   typedef node_traits::const_node_ptr       const_node_ptr;
   typedef MyClass                           value_type;
   typedef MyClass *                         pointer;
   typedef const MyClass *                   const_pointer;
   static const link_mode_type link_mode =   normal_link;

   static node_ptr to_node_ptr (value_type &value)
      {  return node_ptr(&value); }
   static const_node_ptr to_node_ptr (const value_type &value)
      {  return const_node_ptr(&value); }
   static pointer to_value_ptr(node_ptr n)
      {  return static_cast<value_type*>(n); }
   static const_pointer to_value_ptr(const_node_ptr n)
      {  return static_cast<const value_type*>(n); }
};

//Base
typedef base_hook< unordered_set_base_hook<> >  BaseHook;
typedef unordered_bucket<BaseHook>::type        BaseBucketType;
typedef unordered_set<MyClass, BaseHook>        BaseUset;
//Member
typedef member_hook
   < MyClass, unordered_set_member_hook<>
   , &MyClass::member_hook_ >                   MemberHook;
typedef unordered_bucket<MemberHook>::type      MemberBucketType;
typedef unordered_set<MyClass, MemberHook>      MemberUset;
//Explicit
typedef value_traits< uset_value_traits >       Traits;
typedef unordered_bucket<Traits>::type          TraitsBucketType;
typedef unordered_set<MyClass, Traits>          TraitsUset;

struct uset_bucket_traits
{
   //Power of two bucket length
   static const std::size_t NumBuckets = 128;

   uset_bucket_traits(BaseBucketType *buckets)
      :  buckets_(buckets)
   {}

   uset_bucket_traits(const uset_bucket_traits &other)
      :  buckets_(other.buckets_)
   {}

   BaseBucketType * bucket_begin() const
   {  return buckets_;  }

   std::size_t bucket_count() const
   {  return NumBuckets;  }

   BaseBucketType *buckets_;
};

typedef unordered_set
   <MyClass, bucket_traits<uset_bucket_traits>, power_2_buckets<true> >
      BucketTraitsUset;

int main()
{
   if(!detail::is_same<BaseUset::bucket_type, BaseBucketType>::value)
      return 1;
   if(!detail::is_same<MemberUset::bucket_type, MemberBucketType>::value)
      return 1;
   if(!detail::is_same<TraitsUset::bucket_type, TraitsBucketType>::value)
      return 1;
   if(!detail::is_same<BaseBucketType, MemberBucketType>::value)
      return 1;
   if(!detail::is_same<BaseBucketType, TraitsBucketType>::value)
      return 1;

   typedef std::vector<MyClass>::iterator VectIt;
   typedef std::vector<MyClass>::reverse_iterator VectRit;
   std::vector<MyClass> values;

   for(int i = 0; i < 100; ++i)  values.push_back(MyClass(i));

   BaseBucketType buckets[uset_bucket_traits::NumBuckets];
   uset_bucket_traits btraits(buckets);
   BucketTraitsUset uset(btraits);

   for(VectIt it(values.begin()), itend(values.end()); it != itend; ++it)
      uset.insert(*it);

   for( VectRit it(values.rbegin()), itend(values.rend()); it != itend; ++it){
      if(uset.find(*it) == uset.cend())  return 1;
   }

   return 0;
}
