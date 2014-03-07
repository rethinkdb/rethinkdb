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
#include <boost/intrusive/parent_from_member.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/functional/hash.hpp>

using namespace boost::intrusive;

struct MyClass
{
   MyClass() : order(0) {}
   int order;

   //This internal type has two hooks
   struct InnerNode : public list_base_hook<>, public slist_base_hook<>
                    , public set_base_hook<>,  public unordered_set_base_hook<>
   {
      list_member_hook<>            listhook;
      slist_member_hook<>           slisthook;
      set_member_hook<>             sethook;
      unordered_set_member_hook<>   usethook;
   } inner;

   friend bool operator < (const MyClass &l, const MyClass &r)
      { return l.order < r.order; }
   friend bool operator == (const MyClass &l, const MyClass &r)
      { return l.order == r.order; }
   friend std::size_t hash_value(const MyClass &value)
      {  return std::size_t(value.order); }
};

//This functor converts between MyClass and the InnerNode member hook
#define InnerMemberHook(TAG, HOOKTYPE, MEMBERNAME)\
   struct InnerMemberHookFunctor##TAG \
   {\
      typedef HOOKTYPE              hook_type;\
      typedef hook_type*            hook_ptr;\
      typedef const hook_type*      const_hook_ptr;\
      typedef MyClass               value_type;\
      typedef value_type*           pointer;\
      typedef const value_type*     const_pointer;\
                                                \
      static hook_ptr to_hook_ptr (value_type &value)\
         {  return &value.inner.MEMBERNAME; }\
      static const_hook_ptr to_hook_ptr(const value_type &value)\
         {  return &value.inner.MEMBERNAME; }\
      static pointer to_value_ptr(hook_ptr n)\
      {\
         return get_parent_from_member<MyClass>\
            (get_parent_from_member<MyClass::InnerNode>(n, &MyClass::InnerNode::MEMBERNAME)\
            ,&MyClass::inner\
         );\
      }\
      static const_pointer to_value_ptr(const_hook_ptr n)\
      {\
         return get_parent_from_member<MyClass>\
            (get_parent_from_member<MyClass::InnerNode>(n, &MyClass::InnerNode::MEMBERNAME)\
            ,&MyClass::inner\
         );\
      }\
   };\
//


//This functor converts between MyClass and the InnerNode base hook
#define InnerBaseHook(TAG, HOOKTYPE)\
   struct InnerBaseHookFunctor##TAG \
   {\
      typedef HOOKTYPE              hook_type;\
      typedef hook_type*            hook_ptr;\
      typedef const hook_type*      const_hook_ptr;\
      typedef MyClass               value_type;\
      typedef value_type*           pointer;\
      typedef const value_type*     const_pointer;\
                                                \
      static hook_ptr to_hook_ptr (value_type &value)\
         {  return &value.inner; }\
      static const_hook_ptr to_hook_ptr(const value_type &value)\
         {  return &value.inner; }\
      static pointer to_value_ptr(hook_ptr n)\
      {\
         return get_parent_from_member<MyClass>(static_cast<MyClass::InnerNode*>(n),&MyClass::inner);\
      }\
      static const_pointer to_value_ptr(const_hook_ptr n)\
      {\
         return get_parent_from_member<MyClass>(static_cast<const MyClass::InnerNode*>(n),&MyClass::inner);\
      }\
   };\
//

//List
InnerMemberHook(List, list_member_hook<>, listhook)
InnerBaseHook(List, list_base_hook<>)
//Slist
InnerMemberHook(Slist, slist_member_hook<>, slisthook)
InnerBaseHook(Slist, slist_base_hook<>)
//Set
InnerMemberHook(Set, set_member_hook<>, sethook)
InnerBaseHook(Set, set_base_hook<>)
//Unordered Set
InnerMemberHook(USet, unordered_set_member_hook<>, usethook)
InnerBaseHook(USet, unordered_set_base_hook<>)

//Define containers
typedef list < MyClass, function_hook< InnerMemberHookFunctorList> >         CustomListMember;
typedef list < MyClass, function_hook< InnerBaseHookFunctorList  > >         CustomListBase;
typedef slist< MyClass, function_hook< InnerMemberHookFunctorSlist> >        CustomSlistMember;
typedef slist< MyClass, function_hook< InnerBaseHookFunctorSlist  > >        CustomSlistBase;
typedef set  < MyClass, function_hook< InnerMemberHookFunctorSet> >          CustomSetMember;
typedef set  < MyClass, function_hook< InnerBaseHookFunctorSet  > >          CustomSetBase;
typedef unordered_set< MyClass, function_hook< InnerMemberHookFunctorUSet> > CustomUSetMember;
typedef unordered_set< MyClass, function_hook< InnerBaseHookFunctorUSet  > > CustomUSetBase;

int main()
{
   MyClass n;
   CustomListBase    listbase;
   CustomListMember  listmember;
   CustomSlistBase   slistbase;
   CustomSlistMember slistmember;
   CustomSetBase     setbase;
   CustomSetMember   setmember;
   CustomUSetBase::bucket_type buckets[1];
   CustomUSetBase    usetbase(CustomUSetBase::bucket_traits(buckets, 1));
   CustomUSetMember  usetmember(CustomUSetMember::bucket_traits(buckets, 1));

   listbase.insert(listbase.begin(), n);
   listmember.insert(listmember.begin(), n);
   slistbase.insert(slistbase.begin(), n);
   slistmember.insert(slistmember.begin(), n);
   setbase.insert(n);
   setmember.insert(n);
   usetbase.insert(n);
   usetmember.insert(n);

   return 0;
}

#include <boost/intrusive/detail/config_end.hpp>
