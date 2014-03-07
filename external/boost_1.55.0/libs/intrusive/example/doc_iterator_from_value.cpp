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
//[doc_iterator_from_value
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/functional/hash.hpp>
#include <vector>

using namespace boost::intrusive;

class intrusive_data
{
   int data_id_;
   public:

   void set(int id)  {  data_id_ = id;    }

   //This class can be inserted in an intrusive list
   list_member_hook<>   list_hook_;

   //This class can be inserted in an intrusive unordered_set
   unordered_set_member_hook<>   unordered_set_hook_;

   //Comparison operators
   friend bool operator==(const intrusive_data &a, const intrusive_data &b)
   {  return a.data_id_ == b.data_id_; }

   friend bool operator!=(const intrusive_data &a, const intrusive_data &b)
   {  return a.data_id_ != b.data_id_; }

   //The hash function
   friend std::size_t hash_value(const intrusive_data &i)
   {  return boost::hash<int>()(i.data_id_);  }
};

//Definition of the intrusive list that will hold intrusive_data
typedef member_hook<intrusive_data, list_member_hook<>
        , &intrusive_data::list_hook_> MemberListOption;
typedef list<intrusive_data, MemberListOption> list_t;

//Definition of the intrusive unordered_set that will hold intrusive_data
typedef member_hook
      < intrusive_data, unordered_set_member_hook<>
      , &intrusive_data::unordered_set_hook_> MemberUsetOption;
typedef boost::intrusive::unordered_set
   < intrusive_data, MemberUsetOption> unordered_set_t;

int main()
{
   //Create MaxElem objects
   const int MaxElem = 100;
   std::vector<intrusive_data> nodes(MaxElem);

   //Declare the intrusive containers
   list_t     list;
   unordered_set_t::bucket_type buckets[MaxElem];
   unordered_set_t  unordered_set
      (unordered_set_t::bucket_traits(buckets, MaxElem));

   //Initialize all the nodes
   for(int i = 0; i < MaxElem; ++i) nodes[i].set(i);

   //Now insert them in both intrusive containers
   list.insert(list.end(), nodes.begin(), nodes.end());
   unordered_set.insert(nodes.begin(), nodes.end());

   //Now check the iterator_to function
   list_t::iterator list_it(list.begin());
   for(int i = 0; i < MaxElem; ++i, ++list_it)
      if(list.iterator_to(nodes[i])       != list_it ||
         list_t::s_iterator_to(nodes[i])  != list_it)
         return 1;

   //Now check unordered_set::s_iterator_to (which is a member function)
   //and unordered_set::s_local_iterator_to (which is an static member function)
   unordered_set_t::iterator unordered_set_it(unordered_set.begin());
   for(int i = 0; i < MaxElem; ++i){
      unordered_set_it = unordered_set.find(nodes[i]);
      if(unordered_set.iterator_to(nodes[i]) != unordered_set_it)
         return 1;
      if(*unordered_set.local_iterator_to(nodes[i])      != *unordered_set_it ||
         *unordered_set_t::s_local_iterator_to(nodes[i]) != *unordered_set_it )
         return 1;
   }

   return 0;
}
//]
