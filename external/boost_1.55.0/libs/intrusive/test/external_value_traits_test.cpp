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
#include <boost/intrusive/rbtree.hpp>
#include <boost/intrusive/hashtable.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <functional>
#include <vector>

using namespace boost::intrusive;

class MyClass
{
   public:
   int int_;

   MyClass(int i = 0)
      :  int_(i)
   {}
   friend bool operator > (const MyClass &l, const MyClass &r)
   {  return l.int_ > r.int_; }
   friend bool operator == (const MyClass &l, const MyClass &r)
   {  return l.int_ == r.int_; }
   friend std::size_t hash_value(const MyClass &v)
   {  return boost::hash_value(v.int_); }
};

const int NumElements = 100;

template<class NodeTraits>
struct external_traits
{
   typedef NodeTraits                              node_traits;
   typedef typename node_traits::node              node;
   typedef typename node_traits::node_ptr          node_ptr;
   typedef typename node_traits::const_node_ptr    const_node_ptr;
   typedef MyClass                                 value_type;
   typedef typename pointer_traits<node_ptr>::
      template rebind_pointer<MyClass>::type       pointer;
   typedef typename pointer_traits<node_ptr>::
      template rebind_pointer
         <const MyClass>::type                     const_pointer;
   static const link_mode_type link_mode =      normal_link;

   external_traits(pointer values, std::size_t NumElem)
      :  values_(values),  node_array_(NumElem)
   {}

   node_ptr to_node_ptr (value_type &value)
   {  return (&node_array_[0]) + (&value - values_); }

   const_node_ptr to_node_ptr (const value_type &value) const
   {  return &node_array_[0] + (&value - values_); }

   pointer to_value_ptr(node_ptr n)
   {  return values_ + (n - &node_array_[0]); }

   const_pointer to_value_ptr(const_node_ptr n) const
   {  return values_ + (n - &node_array_[0]); }

   pointer  values_;
   std::vector<node> node_array_;
};

template<class NodeTraits>
struct value_traits_proxy;

template<class T>
struct traits_holder
   :  public T
{};

typedef value_traits_proxy<list_node_traits<void*> >                    list_value_traits_proxy;
typedef value_traits_proxy<slist_node_traits<void*> >                   slist_value_traits_proxy;
typedef value_traits_proxy<rbtree_node_traits<void*> >                  rbtree_value_traits_proxy;
typedef value_traits_proxy<traits_holder<slist_node_traits<void*> > >   hash_value_traits_proxy;

struct uset_bucket_traits
{
   private:
   typedef unordered_bucket<value_traits<external_traits
      <traits_holder<slist_node_traits<void*> > > > >::type                bucket_type;

   //Non-copyable
   uset_bucket_traits(const uset_bucket_traits &other);
   uset_bucket_traits & operator=(const uset_bucket_traits &other);

   public:
   static const std::size_t NumBuckets = 100;

   uset_bucket_traits(){}

   bucket_type * bucket_begin() const
   {  return buckets_;  }

   std::size_t bucket_count() const
   {  return NumBuckets;  }

   mutable bucket_type buckets_[NumBuckets];
};

struct bucket_traits_proxy
{
   static const bool external_bucket_traits =   true;
   typedef uset_bucket_traits                   bucket_traits;

   template<class Container>
   bucket_traits &get_bucket_traits(Container &cont);

   template<class Container>
   const bucket_traits &get_bucket_traits(const Container &cont) const;
};

//Define a list that will store MyClass using the external hook
typedef list<MyClass, value_traits<list_value_traits_proxy> >        List;
//Define a slist that will store MyClass using the external hook
typedef slist<MyClass, value_traits<slist_value_traits_proxy> >      Slist;
//Define a rbtree that will store MyClass using the external hook
typedef rbtree< MyClass
              , value_traits<rbtree_value_traits_proxy>
              , compare<std::greater<MyClass> > > Rbtree;
//Define a hashtable that will store MyClass using the external hook

typedef hashtable< MyClass
                 , value_traits<hash_value_traits_proxy>
                 , bucket_traits<bucket_traits_proxy>
                 > Hash;

template<class NodeTraits>
struct value_traits_proxy
{
   static const bool external_value_traits = true;
   typedef external_traits<NodeTraits> value_traits;

   template<class Container>
   const value_traits &get_value_traits(const Container &cont) const;

   template<class Container>
   value_traits &get_value_traits(Container &cont);
};

struct ContainerHolder
   : public uset_bucket_traits
   , public List
   , public external_traits<list_node_traits<void*> >
   , public Slist
   , public external_traits<slist_node_traits<void*> >
   , public Rbtree
   , public external_traits<rbtree_node_traits<void*> >
   , public Hash
   , public external_traits<traits_holder<slist_node_traits<void*> > >
{
   static const std::size_t NumBucket = 100;
   ContainerHolder(MyClass *values, std::size_t num_elem)
      : uset_bucket_traits()
      , List()
      , external_traits<list_node_traits<void*> >(values, num_elem)
      , Slist()
      , external_traits<slist_node_traits<void*> >(values, num_elem)
      , Rbtree()
      , external_traits<rbtree_node_traits<void*> >(values, num_elem)
      , Hash(Hash::bucket_traits())
      , external_traits<traits_holder<slist_node_traits<void*> > >(values, num_elem)
   {}
};

template<class NodeTraits>
template<class Container>
typename value_traits_proxy<NodeTraits>::value_traits &
   value_traits_proxy<NodeTraits>::get_value_traits(Container &cont)
{  return static_cast<value_traits&>(static_cast<ContainerHolder&>(cont)); }

template<class NodeTraits>
template<class Container>
const typename value_traits_proxy<NodeTraits>::value_traits &
   value_traits_proxy<NodeTraits>::get_value_traits(const Container &cont) const
{  return static_cast<const value_traits&>(static_cast<const ContainerHolder&>(cont)); }

template<class Container>
typename bucket_traits_proxy::bucket_traits &
   bucket_traits_proxy::get_bucket_traits(Container &cont)
{  return static_cast<bucket_traits&>(static_cast<ContainerHolder&>(cont)); }

template<class Container>
const typename bucket_traits_proxy::bucket_traits &
   bucket_traits_proxy::get_bucket_traits(const Container &cont) const
{  return static_cast<const bucket_traits&>(static_cast<const ContainerHolder&>(cont)); }

int main()
{
   MyClass  values    [NumElements];
   //Create several MyClass objects, each one with a different value
   for(int i = 0; i < NumElements; ++i)
      values[i].int_ = i;

   ContainerHolder cont_holder(values, NumElements);
   List &my_list     = static_cast<List &>   (cont_holder);
   Slist &my_slist   = static_cast<Slist &>  (cont_holder);
   Rbtree &my_rbtree = static_cast<Rbtree &> (cont_holder);
   Hash     &my_hash = static_cast<Hash &>   (cont_holder);

   //Now insert them in containers
   for(MyClass * it(&values[0]), *itend(&values[NumElements])
      ; it != itend; ++it){
      my_list.push_front(*it);
      my_slist.push_front(*it);
      my_rbtree.insert_unique(*it);
      my_hash.insert_unique(*it);
   }

   //Now test containers
   {
      List::const_iterator   list_it   (my_list.cbegin());
      Slist::const_iterator  slist_it  (my_slist.cbegin());
      Rbtree::const_iterator rbtree_it (my_rbtree.cbegin());
      Hash::const_iterator   hash_it   (my_hash.cbegin());
      MyClass *it_val(&values[NumElements] - 1), *it_rbeg_val(&values[0]-1);

      //Test inserted objects
      for(; it_val != it_rbeg_val; --it_val, ++list_it, ++slist_it, ++rbtree_it){
         if(&*list_it   != &*it_val)   return 1;
         if(&*slist_it  != &*it_val)   return 1;
         if(&*rbtree_it != &*it_val)   return 1;
         hash_it = my_hash.find(*it_val);
         if(hash_it == my_hash.cend() || &*hash_it != &*it_val)
            return 1;
      }
   }

   return 0;
}
