/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2010-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
#include <boost/intrusive/detail/config_begin.hpp>
//[doc_recursive_member
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/parent_from_member.hpp>

using namespace boost::intrusive;

class Recursive;

//Declaration of the functor that converts betwen the Recursive
//class and the hook
struct Functor
{
   //Required types
   typedef list_member_hook<>    hook_type;
   typedef hook_type*            hook_ptr;
   typedef const hook_type*      const_hook_ptr;
   typedef Recursive             value_type;
   typedef value_type*           pointer;
   typedef const value_type*     const_pointer;

   //Required static functions
   static hook_ptr to_hook_ptr (value_type &value);
   static const_hook_ptr to_hook_ptr(const value_type &value);
   static pointer to_value_ptr(hook_ptr n);
   static const_pointer to_value_ptr(const_hook_ptr n);
};

//Define the recursive class
class Recursive
{
   private:
   Recursive(const Recursive&);
   Recursive & operator=(const Recursive&);

   public:
   Recursive() : hook(), children() {}
   list_member_hook<> hook;
   list< Recursive, function_hook< Functor> > children;
};

//Definition of Functor functions
inline Functor::hook_ptr Functor::to_hook_ptr (Functor::value_type &value)
   {  return &value.hook; }
inline Functor::const_hook_ptr Functor::to_hook_ptr(const Functor::value_type &value)
   {  return &value.hook; }
inline Functor::pointer Functor::to_value_ptr(Functor::hook_ptr n)
   {  return get_parent_from_member<Recursive>(n, &Recursive::hook);  }
inline Functor::const_pointer Functor::to_value_ptr(Functor::const_hook_ptr n)
   {  return get_parent_from_member<Recursive>(n, &Recursive::hook);  }

int main()
{
   Recursive f, f2;
   //A recursive list of Recursive
   list< Recursive, function_hook< Functor> > l;

   //Insert a node in parent list
   l.insert(l.begin(), f);

   //Insert a node in child list
   l.begin()->children.insert(l.begin()->children.begin(), f2);

   //Objects properly inserted
   assert(l.size() == l.begin()->children.size());
   assert(l.size() == 1);

   //Clear both lists
   l.begin()->children.clear();
   l.clear();
   return 0;
}
//]

#include <boost/intrusive/detail/config_end.hpp>
