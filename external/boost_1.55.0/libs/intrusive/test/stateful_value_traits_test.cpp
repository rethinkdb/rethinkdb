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
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/functional/hash.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <vector>

using namespace boost::intrusive;

class MyClass
{
   public:
   int int_;

   MyClass(int i = 0)
      :  int_(i)
   {}

   friend bool operator<(const MyClass &l, const MyClass &r)
   {  return l.int_ < r.int_; }

   friend bool operator==(const MyClass &l, const MyClass &r)
   {  return l.int_ == r.int_; }

   friend std::size_t hash_value(const MyClass &v)
   {  return boost::hash_value(v.int_); }
};

template<class T, class NodeTraits>
struct stateful_value_traits
{
   typedef NodeTraits                                          node_traits;
   typedef typename node_traits::node                          node;
   typedef typename node_traits::node_ptr                      node_ptr;
   typedef typename node_traits::const_node_ptr                const_node_ptr;
   typedef T                                                   value_type;
   typedef typename pointer_traits<node_ptr>::
      template rebind_pointer<T>::type                         pointer;
   typedef typename pointer_traits<node_ptr>::
      template rebind_pointer<const T>::type                   const_pointer;

   static const link_mode_type link_mode = normal_link;

   stateful_value_traits(pointer vals, node_ptr node_array)
      :  values_(vals),  node_array_(node_array)
   {}

   node_ptr to_node_ptr (value_type &value)
   {  return node_array_ + (&value - values_); }

   const_node_ptr to_node_ptr (const value_type &value) const
   {  return node_array_ + (&value - values_); }

   pointer to_value_ptr(node_ptr n)
   {  return values_ + (n - node_array_); }

   const_pointer to_value_ptr(const_node_ptr n) const
   {  return values_ + (n - node_array_); }

   pointer  values_;
   node_ptr node_array_;
};

//Define a list that will store MyClass using the external hook
typedef stateful_value_traits< MyClass, list_node_traits<void*> > list_traits;
typedef list<MyClass, value_traits<list_traits> > List;

//Define a slist that will store MyClass using the external hook
typedef stateful_value_traits< MyClass, slist_node_traits<void*> > slist_traits;
typedef slist<MyClass, value_traits<slist_traits> > Slist;

//Define a set that will store MyClass using the external hook
typedef stateful_value_traits< MyClass, rbtree_node_traits<void*> > rbtree_traits;
typedef set<MyClass, value_traits<rbtree_traits> > Set;

//uset uses the same traits as slist
typedef unordered_set<MyClass, value_traits<slist_traits> > Uset;


typedef list_traits::node     list_node_t;
typedef slist_traits::node    slist_node_t;
typedef rbtree_traits::node   rbtree_node_t;

const int NumElements = 100;

MyClass        values    [NumElements];
list_node_t    list_hook_array   [NumElements];
slist_node_t   slist_hook_array  [NumElements];
rbtree_node_t  rbtree_hook_array [NumElements];
slist_node_t   uset_hook_array   [NumElements];

int main()
{
   //Create several MyClass objects, each one with a different value
   for(int i = 0; i < NumElements; ++i)
      values[i].int_ = i;

   Uset::bucket_type buckets[NumElements];

   List  my_list (list_traits (values, list_hook_array));
   Slist my_slist(slist_traits(values, slist_hook_array));
   Set   my_set  (std::less<MyClass>(), rbtree_traits(values, rbtree_hook_array));
   Uset  my_uset ( Uset::bucket_traits(buckets, NumElements)
                 , boost::hash<MyClass>()
                 , std::equal_to<MyClass>()
                 , slist_traits(values, uset_hook_array)
                 );

   //Now insert them in containers
   for(MyClass * it(&values[0]), *itend(&values[NumElements])
      ; it != itend
      ; ++it){
      my_list.push_front(*it);
      my_slist.push_front(*it);
      my_set.insert(*it);
      my_uset.insert(*it);
   }

   //Now test lists
   {
      List::const_iterator   list_it (my_list.cbegin());
      Slist::const_iterator  slist_it(my_slist.cbegin());
      Set::const_reverse_iterator set_rit(my_set.crbegin());
      MyClass *it_val(&values[NumElements-1]), *it_rbeg_val(&values[0]-1);

      //Test the objects inserted in the base hook list
      for(; it_val != it_rbeg_val; --it_val, ++list_it, ++slist_it, ++set_rit){
         if(&*list_it  != &*it_val)   return 1;
         if(&*slist_it != &*it_val)   return 1;
         if(&*set_rit  != &*it_val)   return 1;
         if(my_uset.find(*it_val) == my_uset.cend())  return 1;
      }
   }

   return 0;
}
