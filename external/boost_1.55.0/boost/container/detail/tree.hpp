//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINER_TREE_HPP
#define BOOST_CONTAINER_TREE_HPP

#include "config_begin.hpp"
#include <boost/container/detail/workaround.hpp>
#include <boost/container/container_fwd.hpp>

#include <boost/move/utility.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/intrusive/rbtree.hpp>
#include <boost/container/detail/utilities.hpp>
#include <boost/container/detail/iterators.hpp>
#include <boost/container/detail/algorithms.hpp>
#include <boost/container/detail/node_alloc_holder.hpp>
#include <boost/container/detail/destroyers.hpp>
#include <boost/container/detail/pair.hpp>
#include <boost/container/detail/type_traits.hpp>
#include <boost/container/allocator_traits.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#ifndef BOOST_CONTAINER_PERFECT_FORWARDING
#include <boost/container/detail/preprocessor.hpp>
#endif

#include <utility>   //std::pair
#include <iterator>
#include <algorithm>

namespace boost {
namespace container {
namespace container_detail {

template<class Key, class Value, class KeyCompare, class KeyOfValue>
struct tree_value_compare
   :  public KeyCompare
{
   typedef Value        value_type;
   typedef KeyCompare   key_compare;
   typedef KeyOfValue   key_of_value;
   typedef Key          key_type;

   explicit tree_value_compare(const key_compare &kcomp)
      :  KeyCompare(kcomp)
   {}

   tree_value_compare()
      :  KeyCompare()
   {}

   const key_compare &key_comp() const
   {  return static_cast<const key_compare &>(*this);  }

   key_compare &key_comp()
   {  return static_cast<key_compare &>(*this);  }

   template<class T>
   struct is_key
   {
      static const bool value = is_same<const T, const key_type>::value;
   };

   template<class T>
   typename enable_if_c<is_key<T>::value, const key_type &>::type
      key_forward(const T &key) const
   {  return key; }

   template<class T>
   typename enable_if_c<!is_key<T>::value, const key_type &>::type
      key_forward(const T &key) const
   {  return KeyOfValue()(key);  }

   template<class KeyType, class KeyType2>
   bool operator()(const KeyType &key1, const KeyType2 &key2) const
   {  return key_compare::operator()(this->key_forward(key1), this->key_forward(key2));  }
};

template<class VoidPointer>
struct rbtree_hook
{
   typedef typename container_detail::bi::make_set_base_hook
      < container_detail::bi::void_pointer<VoidPointer>
      , container_detail::bi::link_mode<container_detail::bi::normal_link>
      , container_detail::bi::optimize_size<true>
      >::type  type;
};

//This trait is used to type-pun std::pair because in C++03
//compilers std::pair is useless for C++11 features
template<class T>
struct rbtree_internal_data_type
{
   typedef T type;
};

template<class T1, class T2>
struct rbtree_internal_data_type< std::pair<T1, T2> >
{
   typedef pair<T1, T2> type;
};


//The node to be store in the tree
template <class T, class VoidPointer>
struct rbtree_node
   :  public rbtree_hook<VoidPointer>::type
{
   private:
   //BOOST_COPYABLE_AND_MOVABLE(rbtree_node)
   rbtree_node();

   public:
   typedef typename rbtree_hook<VoidPointer>::type hook_type;

   typedef T value_type;
   typedef typename rbtree_internal_data_type<T>::type internal_type;

   typedef rbtree_node<T, VoidPointer> node_type;

   T &get_data()
   {
      T* ptr = reinterpret_cast<T*>(&this->m_data);
      return *ptr;
   }

   const T &get_data() const
   {
      const T* ptr = reinterpret_cast<const T*>(&this->m_data);
      return *ptr;
   }

   internal_type m_data;

   template<class A, class B>
   void do_assign(const std::pair<const A, B> &p)
   {
      const_cast<A&>(m_data.first) = p.first;
      m_data.second  = p.second;
   }

   template<class A, class B>
   void do_assign(const pair<const A, B> &p)
   {
      const_cast<A&>(m_data.first) = p.first;
      m_data.second  = p.second;
   }

   template<class V>
   void do_assign(const V &v)
   {  m_data = v; }

   template<class A, class B>
   void do_move_assign(std::pair<const A, B> &p)
   {
      const_cast<A&>(m_data.first) = ::boost::move(p.first);
      m_data.second = ::boost::move(p.second);
   }

   template<class A, class B>
   void do_move_assign(pair<const A, B> &p)
   {
      const_cast<A&>(m_data.first) = ::boost::move(p.first);
      m_data.second  = ::boost::move(p.second);
   }

   template<class V>
   void do_move_assign(V &v)
   {  m_data = ::boost::move(v); }
};

template<class Node, class Icont>
class insert_equal_end_hint_functor
{
   Icont &icont_;

   public:
   insert_equal_end_hint_functor(Icont &icont)
      :  icont_(icont)
   {}

   void operator()(Node &n)
   {  this->icont_.insert_equal(this->icont_.cend(), n); }
};

template<class Node, class Icont>
class push_back_functor
{
   Icont &icont_;

   public:
   push_back_functor(Icont &icont)
      :  icont_(icont)
   {}

   void operator()(Node &n)
   {  this->icont_.push_back(n); }
};

}//namespace container_detail {

namespace container_detail {

template<class A, class ValueCompare>
struct intrusive_rbtree_type
{
   typedef typename boost::container::
      allocator_traits<A>::value_type              value_type;
   typedef typename boost::container::
      allocator_traits<A>::void_pointer            void_pointer;
   typedef typename boost::container::
      allocator_traits<A>::size_type               size_type;
   typedef typename container_detail::rbtree_node
         <value_type, void_pointer>                node_type;
   typedef node_compare<ValueCompare, node_type>   node_compare_type;
   typedef typename container_detail::bi::make_rbtree
      <node_type
      ,container_detail::bi::compare<node_compare_type>
      ,container_detail::bi::base_hook<typename rbtree_hook<void_pointer>::type>
      ,container_detail::bi::constant_time_size<true>
      ,container_detail::bi::size_type<size_type>
      >::type                                      container_type;
   typedef container_type                          type ;
};

}  //namespace container_detail {

namespace container_detail {

template <class Key, class Value, class KeyOfValue,
          class KeyCompare, class A>
class rbtree
   : protected container_detail::node_alloc_holder
      < A
      , typename container_detail::intrusive_rbtree_type
         <A, tree_value_compare<Key, Value, KeyCompare, KeyOfValue> 
         >::type
      , tree_value_compare<Key, Value, KeyCompare, KeyOfValue> 
      >
{
   typedef tree_value_compare
            <Key, Value, KeyCompare, KeyOfValue>            ValComp;
   typedef typename container_detail::intrusive_rbtree_type
         < A, ValComp>::type                                Icont;
   typedef container_detail::node_alloc_holder 
      <A, Icont, ValComp>                                   AllocHolder;
   typedef typename AllocHolder::NodePtr                    NodePtr;
   typedef rbtree < Key, Value, KeyOfValue
                  , KeyCompare, A>                          ThisType;
   typedef typename AllocHolder::NodeAlloc                  NodeAlloc;
   typedef typename AllocHolder::ValAlloc                   ValAlloc;
   typedef typename AllocHolder::Node                       Node;
   typedef typename Icont::iterator                         iiterator;
   typedef typename Icont::const_iterator                   iconst_iterator;
   typedef container_detail::allocator_destroyer<NodeAlloc> Destroyer;
   typedef typename AllocHolder::allocator_v1               allocator_v1;
   typedef typename AllocHolder::allocator_v2               allocator_v2;
   typedef typename AllocHolder::alloc_version              alloc_version;

   class RecyclingCloner;
   friend class RecyclingCloner;

   class RecyclingCloner
   {
      public:
      RecyclingCloner(AllocHolder &holder, Icont &irbtree)
         :  m_holder(holder), m_icont(irbtree)
      {}

      NodePtr operator()(const Node &other) const
      {
         if(NodePtr p = m_icont.unlink_leftmost_without_rebalance()){
            //First recycle a node (this can't throw)
            BOOST_TRY{
               //This can throw
               p->do_assign(other.m_data);
               return p;
            }
            BOOST_CATCH(...){
               //If there is an exception destroy the whole source
               m_holder.destroy_node(p);
               while((p = m_icont.unlink_leftmost_without_rebalance())){
                  m_holder.destroy_node(p);
               }
               BOOST_RETHROW
            }
            BOOST_CATCH_END
         }
         else{
            return m_holder.create_node(other.m_data);
         }
      }

      AllocHolder &m_holder;
      Icont &m_icont;
   };

   class RecyclingMoveCloner;
   friend class RecyclingMoveCloner;

   class RecyclingMoveCloner
   {
      public:
      RecyclingMoveCloner(AllocHolder &holder, Icont &irbtree)
         :  m_holder(holder), m_icont(irbtree)
      {}

      NodePtr operator()(const Node &other) const
      {
         if(NodePtr p = m_icont.unlink_leftmost_without_rebalance()){
            //First recycle a node (this can't throw)
            BOOST_TRY{
               //This can throw
               p->do_move_assign(const_cast<Node &>(other).m_data);
               return p;
            }
            BOOST_CATCH(...){
               //If there is an exception destroy the whole source
               m_holder.destroy_node(p);
               while((p = m_icont.unlink_leftmost_without_rebalance())){
                  m_holder.destroy_node(p);
               }
               BOOST_RETHROW
            }
            BOOST_CATCH_END
         }
         else{
            return m_holder.create_node(other.m_data);
         }
      }

      AllocHolder &m_holder;
      Icont &m_icont;
   };

   BOOST_COPYABLE_AND_MOVABLE(rbtree)

   public:

   typedef Key                                        key_type;
   typedef Value                                      value_type;
   typedef A                                          allocator_type;
   typedef KeyCompare                                 key_compare;
   typedef ValComp                                    value_compare;
   typedef typename boost::container::
      allocator_traits<A>::pointer                    pointer;
   typedef typename boost::container::
      allocator_traits<A>::const_pointer              const_pointer;
   typedef typename boost::container::
      allocator_traits<A>::reference                  reference;
   typedef typename boost::container::
      allocator_traits<A>::const_reference            const_reference;
   typedef typename boost::container::
      allocator_traits<A>::size_type                  size_type;
   typedef typename boost::container::
      allocator_traits<A>::difference_type            difference_type;
   typedef difference_type                            rbtree_difference_type;
   typedef pointer                                    rbtree_pointer;
   typedef const_pointer                              rbtree_const_pointer;
   typedef reference                                  rbtree_reference;
   typedef const_reference                            rbtree_const_reference;
   typedef NodeAlloc                                  stored_allocator_type;

   private:

   template<class KeyValueCompare>
   struct key_node_compare
      :  private KeyValueCompare
   {
      key_node_compare(const KeyValueCompare &comp)
         :  KeyValueCompare(comp)
      {}

      template<class T>
      struct is_node
      {
         static const bool value = is_same<T, Node>::value;
      };

      template<class T>
      typename enable_if_c<is_node<T>::value, const value_type &>::type
         key_forward(const T &node) const
      {  return node.get_data();  }

      template<class T>
      typename enable_if_c<!is_node<T>::value, const T &>::type
         key_forward(const T &key) const
      {  return key; }

      template<class KeyType, class KeyType2>
      bool operator()(const KeyType &key1, const KeyType2 &key2) const
      {  return KeyValueCompare::operator()(this->key_forward(key1), this->key_forward(key2));  }
   };

   typedef key_node_compare<value_compare>  KeyNodeCompare;

   public:
   typedef container_detail::iterator<iiterator, false>  iterator;
   typedef container_detail::iterator<iiterator, true >  const_iterator;
   typedef std::reverse_iterator<iterator>        reverse_iterator;
   typedef std::reverse_iterator<const_iterator>  const_reverse_iterator;

   rbtree()
      : AllocHolder(ValComp(key_compare()))
   {}

   explicit rbtree(const key_compare& comp, const allocator_type& a = allocator_type())
      : AllocHolder(a, ValComp(comp))
   {}

   explicit rbtree(const allocator_type& a)
      : AllocHolder(a)
   {}

   template <class InputIterator>
   rbtree(bool unique_insertion, InputIterator first, InputIterator last, const key_compare& comp,
          const allocator_type& a
      #if !defined(BOOST_CONTAINER_DOXYGEN_INVOKED)
      , typename container_detail::enable_if_c
         < container_detail::is_input_iterator<InputIterator>::value
            || container_detail::is_same<alloc_version, allocator_v1>::value
         >::type * = 0
      #endif
         )
      : AllocHolder(a, value_compare(comp))
   {
      //Use cend() as hint to achieve linear time for
      //ordered ranges as required by the standard
      //for the constructor
      const const_iterator end_it(this->cend());
      if(unique_insertion){
         for ( ; first != last; ++first){
            this->insert_unique(end_it, *first);
         }
      }
      else{
         for ( ; first != last; ++first){
            this->insert_equal(end_it, *first);
         }
      }
   }

   template <class InputIterator>
   rbtree(bool unique_insertion, InputIterator first, InputIterator last, const key_compare& comp,
          const allocator_type& a
      #if !defined(BOOST_CONTAINER_DOXYGEN_INVOKED)
      , typename container_detail::enable_if_c
         < !(container_detail::is_input_iterator<InputIterator>::value
            || container_detail::is_same<alloc_version, allocator_v1>::value)
         >::type * = 0
      #endif
         )
      : AllocHolder(a, value_compare(comp))
   {
      if(unique_insertion){
         //Use cend() as hint to achieve linear time for
         //ordered ranges as required by the standard
         //for the constructor
         const const_iterator end_it(this->cend());
         for ( ; first != last; ++first){
            this->insert_unique(end_it, *first);
         }
      }
      else{
         //Optimized allocation and construction
         this->allocate_many_and_construct
            ( first, std::distance(first, last)
            , insert_equal_end_hint_functor<Node, Icont>(this->icont()));
      }
   }

   template <class InputIterator>
   rbtree( ordered_range_t, InputIterator first, InputIterator last
         , const key_compare& comp = key_compare(), const allocator_type& a = allocator_type()
         #if !defined(BOOST_CONTAINER_DOXYGEN_INVOKED)
         , typename container_detail::enable_if_c
            < container_detail::is_input_iterator<InputIterator>::value
               || container_detail::is_same<alloc_version, allocator_v1>::value
            >::type * = 0
         #endif
         )
      : AllocHolder(a, value_compare(comp))
   {
      for ( ; first != last; ++first){
         this->push_back_impl(*first);
      }
   }

   template <class InputIterator>
   rbtree( ordered_range_t, InputIterator first, InputIterator last
         , const key_compare& comp = key_compare(), const allocator_type& a = allocator_type()
         #if !defined(BOOST_CONTAINER_DOXYGEN_INVOKED)
         , typename container_detail::enable_if_c
            < !(container_detail::is_input_iterator<InputIterator>::value
               || container_detail::is_same<alloc_version, allocator_v1>::value)
            >::type * = 0
         #endif
         )
      : AllocHolder(a, value_compare(comp))
   {
      //Optimized allocation and construction
      this->allocate_many_and_construct
         ( first, std::distance(first, last)
         , container_detail::push_back_functor<Node, Icont>(this->icont()));
   }

   rbtree(const rbtree& x)
      :  AllocHolder(x, x.value_comp())
   {
      this->icont().clone_from
         (x.icont(), typename AllocHolder::cloner(*this), Destroyer(this->node_alloc()));
   }

   rbtree(BOOST_RV_REF(rbtree) x)
      :  AllocHolder(::boost::move(static_cast<AllocHolder&>(x)), x.value_comp())
   {}

   rbtree(const rbtree& x, const allocator_type &a)
      :  AllocHolder(a, x.value_comp())
   {
      this->icont().clone_from
         (x.icont(), typename AllocHolder::cloner(*this), Destroyer(this->node_alloc()));
   }

   rbtree(BOOST_RV_REF(rbtree) x, const allocator_type &a)
      :  AllocHolder(a, x.value_comp())
   {
      if(this->node_alloc() == x.node_alloc()){
         this->icont().swap(x.icont());
      }
      else{
         this->icont().clone_from
            (x.icont(), typename AllocHolder::cloner(*this), Destroyer(this->node_alloc()));
      }
   }

   ~rbtree()
   {} //AllocHolder clears the tree

   rbtree& operator=(BOOST_COPY_ASSIGN_REF(rbtree) x)
   {
      if (&x != this){
         NodeAlloc &this_alloc     = this->get_stored_allocator();
         const NodeAlloc &x_alloc  = x.get_stored_allocator();
         container_detail::bool_<allocator_traits<NodeAlloc>::
            propagate_on_container_copy_assignment::value> flag;
         if(flag && this_alloc != x_alloc){
            this->clear();
         }
         this->AllocHolder::copy_assign_alloc(x);
         //Transfer all the nodes to a temporary tree
         //If anything goes wrong, all the nodes will be destroyed
         //automatically
         Icont other_tree(::boost::move(this->icont()));

         //Now recreate the source tree reusing nodes stored by other_tree
         this->icont().clone_from
            (x.icont()
            , RecyclingCloner(*this, other_tree)
            , Destroyer(this->node_alloc()));

         //If there are remaining nodes, destroy them
         NodePtr p;
         while((p = other_tree.unlink_leftmost_without_rebalance())){
            AllocHolder::destroy_node(p);
         }
      }
      return *this;
   }

   rbtree& operator=(BOOST_RV_REF(rbtree) x)
   {
      if (&x != this){
         NodeAlloc &this_alloc = this->get_stored_allocator();
         const NodeAlloc &x_alloc    = x.get_stored_allocator();
         //If allocators are equal we can just swap pointers
         if(this_alloc == x_alloc){
            //Destroy and swap pointers
            this->clear();
            this->icont() = ::boost::move(x.icont());
            //Move allocator if needed
            this->AllocHolder::move_assign_alloc(x);
         }
         //If unequal allocators, then do a one by one move
         else{
            //Transfer all the nodes to a temporary tree
            //If anything goes wrong, all the nodes will be destroyed
            //automatically
            Icont other_tree(::boost::move(this->icont()));

            //Now recreate the source tree reusing nodes stored by other_tree
            this->icont().clone_from
               (x.icont()
               , RecyclingMoveCloner(*this, other_tree)
               , Destroyer(this->node_alloc()));

            //If there are remaining nodes, destroy them
            NodePtr p;
            while((p = other_tree.unlink_leftmost_without_rebalance())){
               AllocHolder::destroy_node(p);
            }
         }
      }
      return *this;
   }

   public:   
   // accessors:
   value_compare value_comp() const
   {  return this->icont().value_comp().value_comp(); }

   key_compare key_comp() const
   {  return this->icont().value_comp().value_comp().key_comp(); }

   allocator_type get_allocator() const
   {  return allocator_type(this->node_alloc()); }

   const stored_allocator_type &get_stored_allocator() const
   {  return this->node_alloc(); }

   stored_allocator_type &get_stored_allocator()
   {  return this->node_alloc(); }

   iterator begin()
   { return iterator(this->icont().begin()); }

   const_iterator begin() const
   {  return this->cbegin();  }

   iterator end()
   {  return iterator(this->icont().end());  }

   const_iterator end() const
   {  return this->cend();  }

   reverse_iterator rbegin()
   {  return reverse_iterator(end());  }

   const_reverse_iterator rbegin() const
   {  return this->crbegin();  }

   reverse_iterator rend()
   {  return reverse_iterator(begin());   }

   const_reverse_iterator rend() const
   {  return this->crend();   }

   //! <b>Effects</b>: Returns a const_iterator to the first element contained in the container.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   const_iterator cbegin() const
   { return const_iterator(this->non_const_icont().begin()); }

   //! <b>Effects</b>: Returns a const_iterator to the end of the container.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   const_iterator cend() const
   { return const_iterator(this->non_const_icont().end()); }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the beginning
   //! of the reversed container.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   const_reverse_iterator crbegin() const
   { return const_reverse_iterator(cend()); }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the end
   //! of the reversed container.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   const_reverse_iterator crend() const
   { return const_reverse_iterator(cbegin()); }

   bool empty() const
   {  return !this->size();  }

   size_type size() const
   {  return this->icont().size();   }

   size_type max_size() const
   {  return AllocHolder::max_size();  }

   void swap(ThisType& x)
   {  AllocHolder::swap(x);   }

   public:

   typedef typename Icont::insert_commit_data insert_commit_data;

   // insert/erase
   std::pair<iterator,bool> insert_unique_check
      (const key_type& key, insert_commit_data &data)
   {
      std::pair<iiterator, bool> ret =
         this->icont().insert_unique_check(key, KeyNodeCompare(value_comp()), data);
      return std::pair<iterator, bool>(iterator(ret.first), ret.second);
   }

   std::pair<iterator,bool> insert_unique_check
      (const_iterator hint, const key_type& key, insert_commit_data &data)
   {
      std::pair<iiterator, bool> ret =
         this->icont().insert_unique_check(hint.get(), key, KeyNodeCompare(value_comp()), data);
      return std::pair<iterator, bool>(iterator(ret.first), ret.second);
   }

   iterator insert_unique_commit(const value_type& v, insert_commit_data &data)
   {
      NodePtr tmp = AllocHolder::create_node(v);
      scoped_destroy_deallocator<NodeAlloc> destroy_deallocator(tmp, this->node_alloc());
      iterator ret(this->icont().insert_unique_commit(*tmp, data));
      destroy_deallocator.release();
      return ret;
   }

   template<class MovableConvertible>
   iterator insert_unique_commit
      (BOOST_FWD_REF(MovableConvertible) mv, insert_commit_data &data)
   {
      NodePtr tmp = AllocHolder::create_node(boost::forward<MovableConvertible>(mv));
      scoped_destroy_deallocator<NodeAlloc> destroy_deallocator(tmp, this->node_alloc());
      iterator ret(this->icont().insert_unique_commit(*tmp, data));
      destroy_deallocator.release();
      return ret;
   }

   std::pair<iterator,bool> insert_unique(const value_type& v)
   {
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(KeyOfValue()(v), data);
      if(ret.second){
         ret.first = this->insert_unique_commit(v, data);
      }
      return ret;
   }

   template<class MovableConvertible>
   std::pair<iterator,bool> insert_unique(BOOST_FWD_REF(MovableConvertible) mv)
   {
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(KeyOfValue()(mv), data);
      if(ret.second){
         ret.first = this->insert_unique_commit(boost::forward<MovableConvertible>(mv), data);
      }
      return ret;
   }

   private:

   template<class MovableConvertible>
   void push_back_impl(BOOST_FWD_REF(MovableConvertible) mv)
   {
      NodePtr tmp(AllocHolder::create_node(boost::forward<MovableConvertible>(mv)));
      //push_back has no-throw guarantee so avoid any deallocator/destroyer
      this->icont().push_back(*tmp);
   }

   std::pair<iterator, bool> emplace_unique_impl(NodePtr p)
   {
      value_type &v = p->get_data();
      insert_commit_data data;
      scoped_destroy_deallocator<NodeAlloc> destroy_deallocator(p, this->node_alloc());
      std::pair<iterator,bool> ret =
         this->insert_unique_check(KeyOfValue()(v), data);
      if(!ret.second){
         return ret;
      }
      //No throw insertion part, release rollback
      destroy_deallocator.release();
      return std::pair<iterator,bool>
         ( iterator(iiterator(this->icont().insert_unique_commit(*p, data)))
         , true );
   }

   iterator emplace_unique_hint_impl(const_iterator hint, NodePtr p)
   {
      value_type &v = p->get_data();
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(hint, KeyOfValue()(v), data);
      if(!ret.second){
         Destroyer(this->node_alloc())(p);
         return ret.first;
      }
      return iterator(iiterator(this->icont().insert_unique_commit(*p, data)));
   }

   public:

   #ifdef BOOST_CONTAINER_PERFECT_FORWARDING

   template <class... Args>
   std::pair<iterator, bool> emplace_unique(Args&&... args)
   {  return this->emplace_unique_impl(AllocHolder::create_node(boost::forward<Args>(args)...));   }

   template <class... Args>
   iterator emplace_hint_unique(const_iterator hint, Args&&... args)
   {  return this->emplace_unique_hint_impl(hint, AllocHolder::create_node(boost::forward<Args>(args)...));   }

   template <class... Args>
   iterator emplace_equal(Args&&... args)
   {
      NodePtr tmp(AllocHolder::create_node(boost::forward<Args>(args)...));
      scoped_destroy_deallocator<NodeAlloc> destroy_deallocator(tmp, this->node_alloc());
      iterator ret(this->icont().insert_equal(this->icont().end(), *tmp));
      destroy_deallocator.release();
      return ret;
   }

   template <class... Args>
   iterator emplace_hint_equal(const_iterator hint, Args&&... args)
   {
      NodePtr tmp(AllocHolder::create_node(boost::forward<Args>(args)...));
      scoped_destroy_deallocator<NodeAlloc> destroy_deallocator(tmp, this->node_alloc());
      iterator ret(this->icont().insert_equal(hint.get(), *tmp));
      destroy_deallocator.release();
      return ret;
   }

   #else //#ifdef BOOST_CONTAINER_PERFECT_FORWARDING

   #define BOOST_PP_LOCAL_MACRO(n)                                                                          \
   BOOST_PP_EXPR_IF(n, template<) BOOST_PP_ENUM_PARAMS(n, class P) BOOST_PP_EXPR_IF(n, >)                   \
   std::pair<iterator, bool> emplace_unique(BOOST_PP_ENUM(n, BOOST_CONTAINER_PP_PARAM_LIST, _))             \
   {                                                                                                        \
      return this->emplace_unique_impl                                                                      \
         (AllocHolder::create_node(BOOST_PP_ENUM(n, BOOST_CONTAINER_PP_PARAM_FORWARD, _)));                 \
   }                                                                                                        \
                                                                                                            \
   BOOST_PP_EXPR_IF(n, template<) BOOST_PP_ENUM_PARAMS(n, class P) BOOST_PP_EXPR_IF(n, >)                   \
   iterator emplace_hint_unique(const_iterator hint                                                         \
                       BOOST_PP_ENUM_TRAILING(n, BOOST_CONTAINER_PP_PARAM_LIST, _))                         \
   {                                                                                                        \
      return this->emplace_unique_hint_impl                                                                 \
         (hint, AllocHolder::create_node(BOOST_PP_ENUM(n, BOOST_CONTAINER_PP_PARAM_FORWARD, _)));           \
   }                                                                                                        \
                                                                                                            \
   BOOST_PP_EXPR_IF(n, template<) BOOST_PP_ENUM_PARAMS(n, class P) BOOST_PP_EXPR_IF(n, >)                   \
   iterator emplace_equal(BOOST_PP_ENUM(n, BOOST_CONTAINER_PP_PARAM_LIST, _))                               \
   {                                                                                                        \
      NodePtr tmp(AllocHolder::create_node(BOOST_PP_ENUM(n, BOOST_CONTAINER_PP_PARAM_FORWARD, _)));         \
      scoped_destroy_deallocator<NodeAlloc> destroy_deallocator(tmp, this->node_alloc());                   \
      iterator ret(this->icont().insert_equal(this->icont().end(), *tmp));                                  \
      destroy_deallocator.release();                                                                        \
      return ret;                                                                                           \
   }                                                                                                        \
                                                                                                            \
   BOOST_PP_EXPR_IF(n, template<) BOOST_PP_ENUM_PARAMS(n, class P) BOOST_PP_EXPR_IF(n, >)                   \
   iterator emplace_hint_equal(const_iterator hint                                                          \
                       BOOST_PP_ENUM_TRAILING(n, BOOST_CONTAINER_PP_PARAM_LIST, _))                         \
   {                                                                                                        \
      NodePtr tmp(AllocHolder::create_node(BOOST_PP_ENUM(n, BOOST_CONTAINER_PP_PARAM_FORWARD, _)));         \
      scoped_destroy_deallocator<NodeAlloc> destroy_deallocator(tmp, this->node_alloc());                   \
      iterator ret(this->icont().insert_equal(hint.get(), *tmp));                                           \
      destroy_deallocator.release();                                                                        \
      return ret;                                                                                           \
   }                                                                                                        \
   //!
   #define BOOST_PP_LOCAL_LIMITS (0, BOOST_CONTAINER_MAX_CONSTRUCTOR_PARAMETERS)
   #include BOOST_PP_LOCAL_ITERATE()

   #endif   //#ifdef BOOST_CONTAINER_PERFECT_FORWARDING

   iterator insert_unique(const_iterator hint, const value_type& v)
   {
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(hint, KeyOfValue()(v), data);
      if(!ret.second)
         return ret.first;
      return this->insert_unique_commit(v, data);
   }

   template<class MovableConvertible>
   iterator insert_unique(const_iterator hint, BOOST_FWD_REF(MovableConvertible) mv)
   {
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(hint, KeyOfValue()(mv), data);
      if(!ret.second)
         return ret.first;
      return this->insert_unique_commit(boost::forward<MovableConvertible>(mv), data);
   }

   template <class InputIterator>
   void insert_unique(InputIterator first, InputIterator last)
   {
      for( ; first != last; ++first)
         this->insert_unique(*first);
   }

   iterator insert_equal(const value_type& v)
   {
      NodePtr tmp(AllocHolder::create_node(v));
      scoped_destroy_deallocator<NodeAlloc> destroy_deallocator(tmp, this->node_alloc());
      iterator ret(this->icont().insert_equal(this->icont().end(), *tmp));
      destroy_deallocator.release();
      return ret;
   }

   template<class MovableConvertible>
   iterator insert_equal(BOOST_FWD_REF(MovableConvertible) mv)
   {
      NodePtr tmp(AllocHolder::create_node(boost::forward<MovableConvertible>(mv)));
      scoped_destroy_deallocator<NodeAlloc> destroy_deallocator(tmp, this->node_alloc());
      iterator ret(this->icont().insert_equal(this->icont().end(), *tmp));
      destroy_deallocator.release();
      return ret;
   }

   iterator insert_equal(const_iterator hint, const value_type& v)
   {
      NodePtr tmp(AllocHolder::create_node(v));
      scoped_destroy_deallocator<NodeAlloc> destroy_deallocator(tmp, this->node_alloc());
      iterator ret(this->icont().insert_equal(hint.get(), *tmp));
      destroy_deallocator.release();
      return ret;
   }

   template<class MovableConvertible>
   iterator insert_equal(const_iterator hint, BOOST_FWD_REF(MovableConvertible) mv)
   {
      NodePtr tmp(AllocHolder::create_node(boost::forward<MovableConvertible>(mv)));
      scoped_destroy_deallocator<NodeAlloc> destroy_deallocator(tmp, this->node_alloc());
      iterator ret(this->icont().insert_equal(hint.get(), *tmp));
      destroy_deallocator.release();
      return ret;
   }

   template <class InputIterator>
   void insert_equal(InputIterator first, InputIterator last)
   {
      for( ; first != last; ++first)
         this->insert_equal(*first);
   }

   iterator erase(const_iterator position)
   {  return iterator(this->icont().erase_and_dispose(position.get(), Destroyer(this->node_alloc()))); }

   size_type erase(const key_type& k)
   {  return AllocHolder::erase_key(k, KeyNodeCompare(value_comp()), alloc_version()); }

   iterator erase(const_iterator first, const_iterator last)
   {  return iterator(AllocHolder::erase_range(first.get(), last.get(), alloc_version())); }

   void clear()
   {  AllocHolder::clear(alloc_version());  }

   // set operations:
   iterator find(const key_type& k)
   {  return iterator(this->icont().find(k, KeyNodeCompare(value_comp())));  }

   const_iterator find(const key_type& k) const
   {  return const_iterator(this->non_const_icont().find(k, KeyNodeCompare(value_comp())));  }

   size_type count(const key_type& k) const
   {  return size_type(this->icont().count(k, KeyNodeCompare(value_comp()))); }

   iterator lower_bound(const key_type& k)
   {  return iterator(this->icont().lower_bound(k, KeyNodeCompare(value_comp())));  }

   const_iterator lower_bound(const key_type& k) const
   {  return const_iterator(this->non_const_icont().lower_bound(k, KeyNodeCompare(value_comp())));  }

   iterator upper_bound(const key_type& k)
   {  return iterator(this->icont().upper_bound(k, KeyNodeCompare(value_comp())));   }

   const_iterator upper_bound(const key_type& k) const
   {  return const_iterator(this->non_const_icont().upper_bound(k, KeyNodeCompare(value_comp())));  }

   std::pair<iterator,iterator> equal_range(const key_type& k)
   {
      std::pair<iiterator, iiterator> ret =
         this->icont().equal_range(k, KeyNodeCompare(value_comp()));
      return std::pair<iterator,iterator>(iterator(ret.first), iterator(ret.second));
   }

   std::pair<const_iterator, const_iterator> equal_range(const key_type& k) const
   {
      std::pair<iiterator, iiterator> ret =
         this->non_const_icont().equal_range(k, KeyNodeCompare(value_comp()));
      return std::pair<const_iterator,const_iterator>
         (const_iterator(ret.first), const_iterator(ret.second));
   }
};

template <class Key, class Value, class KeyOfValue,
          class KeyCompare, class A>
inline bool
operator==(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x,
           const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y)
{
  return x.size() == y.size() &&
         std::equal(x.begin(), x.end(), y.begin());
}

template <class Key, class Value, class KeyOfValue,
          class KeyCompare, class A>
inline bool
operator<(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x,
          const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y)
{
  return std::lexicographical_compare(x.begin(), x.end(),
                                      y.begin(), y.end());
}

template <class Key, class Value, class KeyOfValue,
          class KeyCompare, class A>
inline bool
operator!=(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x,
           const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y) {
  return !(x == y);
}

template <class Key, class Value, class KeyOfValue,
          class KeyCompare, class A>
inline bool
operator>(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x,
          const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y) {
  return y < x;
}

template <class Key, class Value, class KeyOfValue,
          class KeyCompare, class A>
inline bool
operator<=(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x,
           const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y) {
  return !(y < x);
}

template <class Key, class Value, class KeyOfValue,
          class KeyCompare, class A>
inline bool
operator>=(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x,
           const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y) {
  return !(x < y);
}


template <class Key, class Value, class KeyOfValue,
          class KeyCompare, class A>
inline void
swap(rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x,
     rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y)
{
  x.swap(y);
}

} //namespace container_detail {
} //namespace container {
/*
//!has_trivial_destructor_after_move<> == true_type
//!specialization for optimizations
template <class K, class V, class KOV,
class C, class A>
struct has_trivial_destructor_after_move
   <boost::container::container_detail::rbtree<K, V, KOV, C, A> >
{
   static const bool value = has_trivial_destructor_after_move<A>::value && has_trivial_destructor_after_move<C>::value;
};
*/
} //namespace boost  {

#include <boost/container/detail/config_end.hpp>

#endif //BOOST_CONTAINER_TREE_HPP
