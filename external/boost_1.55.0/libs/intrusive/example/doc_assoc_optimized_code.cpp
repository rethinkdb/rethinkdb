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
//[doc_assoc_optimized_code_normal_find
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <cstring>

using namespace boost::intrusive;

// Hash function for strings
struct StrHasher
{
   std::size_t operator()(const char *str) const
   {
      std::size_t seed = 0;
      for(; *str; ++str)   boost::hash_combine(seed, *str);
      return seed;
   }
};

class Expensive : public set_base_hook<>, public unordered_set_base_hook<>
{
   std::string key_;
   // Other members...

   public:
   Expensive(const char *key)
      :  key_(key)
      {}  //other expensive initializations...

   const std::string & get_key() const
      {  return key_;   }

   friend bool operator <  (const Expensive &a, const Expensive &b)
      {  return a.key_ < b.key_;  }

   friend bool operator == (const Expensive &a, const Expensive &b)
      {  return a.key_ == b.key_;  }

   friend std::size_t hash_value(const Expensive &object)
      {  return StrHasher()(object.get_key().c_str());  }
};

// A set and unordered_set that store Expensive objects
typedef set<Expensive>           Set;
typedef unordered_set<Expensive> UnorderedSet;

// Search functions
Expensive *get_from_set(const char* key, Set &set_object)
{
   Set::iterator it = set_object.find(Expensive(key));
   if( it == set_object.end() )     return 0;
   return &*it;
}

Expensive *get_from_uset(const char* key, UnorderedSet &uset_object)
{
   UnorderedSet::iterator it = uset_object.find(Expensive (key));
   if( it == uset_object.end() )  return 0;
   return &*it;
}
//]

//[doc_assoc_optimized_code_optimized_find
// These compare Expensive and a c-string
struct StrExpComp
{
   bool operator()(const char *str, const Expensive &c) const
   {  return std::strcmp(str, c.get_key().c_str()) < 0;  }

   bool operator()(const Expensive &c, const char *str) const
   {  return std::strcmp(c.get_key().c_str(), str) < 0;  }
};

struct StrExpEqual
{
   bool operator()(const char *str, const Expensive &c) const
   {  return std::strcmp(str, c.get_key().c_str()) == 0;  }

   bool operator()(const Expensive &c, const char *str) const
   {  return std::strcmp(c.get_key().c_str(), str) == 0;  }
};

// Optimized search functions
Expensive *get_from_set_optimized(const char* key, Set &set_object)
{
   Set::iterator it = set_object.find(key, StrExpComp());
   if( it == set_object.end() )   return 0;
   return &*it;
}

Expensive *get_from_uset_optimized(const char* key, UnorderedSet &uset_object)
{
   UnorderedSet::iterator it = uset_object.find(key, StrHasher(), StrExpEqual());
   if( it == uset_object.end() )  return 0;
   return &*it;
}
//]

//[doc_assoc_optimized_code_normal_insert
// Insertion functions
bool insert_to_set(const char* key, Set &set_object)
{
   Expensive *pobject = new Expensive(key);
   bool success = set_object.insert(*pobject).second;
   if(!success)   delete pobject;
   return success;
}

bool insert_to_uset(const char* key, UnorderedSet &uset_object)
{
   Expensive *pobject = new Expensive(key);
   bool success = uset_object.insert(*pobject).second;
   if(!success)   delete pobject;
   return success;
}
//]

//[doc_assoc_optimized_code_optimized_insert
// Optimized insertion functions
bool insert_to_set_optimized(const char* key, Set &set_object)
{
   Set::insert_commit_data insert_data;
   bool success = set_object.insert_check(key, StrExpComp(), insert_data).second;
   if(success) set_object.insert_commit(*new Expensive(key), insert_data);
   return success;
}

bool insert_to_uset_optimized(const char* key, UnorderedSet &uset_object)
{
   UnorderedSet::insert_commit_data insert_data;
   bool success = uset_object.insert_check
      (key, StrHasher(), StrExpEqual(), insert_data).second;
   if(success) uset_object.insert_commit(*new Expensive(key), insert_data);
   return success;
}
//]

int main()
{
   Set set;
   UnorderedSet::bucket_type buckets[10];
   UnorderedSet unordered_set(UnorderedSet::bucket_traits(buckets, 10));

   const char * const expensive_key
      = "A long string that avoids small string optimization";

   Expensive value(expensive_key);

   if(get_from_set(expensive_key, set)){
      return 1;
   }

   if(get_from_uset(expensive_key, unordered_set)){
      return 1;
   }

   if(get_from_set_optimized(expensive_key, set)){
      return 1;
   }

   if(get_from_uset_optimized(expensive_key, unordered_set)){
      return 1;
   }

   Set::iterator setit =  set.insert(value).first;
   UnorderedSet::iterator unordered_setit =  unordered_set.insert(value).first;

   if(!get_from_set(expensive_key, set)){
      return 1;
   }

   if(!get_from_uset(expensive_key, unordered_set)){
      return 1;
   }

   if(!get_from_set_optimized(expensive_key, set)){
      return 1;
   }

   if(!get_from_uset_optimized(expensive_key, unordered_set)){
      return 1;
   }

   set.erase(setit);
   unordered_set.erase(unordered_setit);

   if(!insert_to_set(expensive_key, set)){
      return 1;
   }

   if(!insert_to_uset(expensive_key, unordered_set)){
      return 1;
   }

   {
      Expensive *ptr = &*set.begin();
      set.clear();
      delete ptr;
   }

   {
      Expensive *ptr = &*unordered_set.begin();
      unordered_set.clear();
      delete ptr;
   }

   if(!insert_to_set_optimized(expensive_key, set)){
      return 1;
   }

   if(!insert_to_uset_optimized(expensive_key, unordered_set)){
      return 1;
   }

   {
      Expensive *ptr = &*set.begin();
      set.clear();
      delete ptr;
   }

   {
      Expensive *ptr = &*unordered_set.begin();
      unordered_set.clear();
      delete ptr;
   }

   setit       =  set.insert(value).first;
   unordered_setit   =  unordered_set.insert(value).first;

   if(insert_to_set(expensive_key, set)){
      return 1;
   }

   if(insert_to_uset(expensive_key, unordered_set)){
      return 1;
   }

   if(insert_to_set_optimized(expensive_key, set)){
      return 1;
   }

   if(insert_to_uset_optimized(expensive_key, unordered_set)){
      return 1;
   }

   set.erase(value);
   unordered_set.erase(value);

   return 0;
}
