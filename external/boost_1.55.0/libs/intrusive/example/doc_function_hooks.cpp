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
//[doc_function_hooks
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/parent_from_member.hpp>

using namespace boost::intrusive;

struct MyClass
{
   int dummy;
   //This internal type has a member hook
   struct InnerNode
   {
      int dummy;
      list_member_hook<> hook;
   } inner;
};

//This functor converts between MyClass and InnerNode's member hook
struct Functor
{
   //Required types
   typedef list_member_hook<>    hook_type;
   typedef hook_type*            hook_ptr;
   typedef const hook_type*      const_hook_ptr;
   typedef MyClass               value_type;
   typedef value_type*           pointer;
   typedef const value_type*     const_pointer;

   //Required static functions
   static hook_ptr to_hook_ptr (value_type &value)
      {  return &value.inner.hook; }
   static const_hook_ptr to_hook_ptr(const value_type &value)
      {  return &value.inner.hook; }
   static pointer to_value_ptr(hook_ptr n)
   {
      return get_parent_from_member<MyClass>
         (get_parent_from_member<MyClass::InnerNode>(n, &MyClass::InnerNode::hook)
         ,&MyClass::inner
      );
   }
   static const_pointer to_value_ptr(const_hook_ptr n)
   {
      return get_parent_from_member<MyClass>
         (get_parent_from_member<MyClass::InnerNode>(n, &MyClass::InnerNode::hook)
         ,&MyClass::inner
      );
   }
};

//Define a list that will use the hook accessed through the function object
typedef list< MyClass, function_hook< Functor> >  List;

int main()
{
   MyClass n;
   List l;
   //Insert the node in both lists
   l.insert(l.begin(), n);
   assert(l.size() == 1);
   return 0;
}
//]

#include <boost/intrusive/detail/config_end.hpp>
