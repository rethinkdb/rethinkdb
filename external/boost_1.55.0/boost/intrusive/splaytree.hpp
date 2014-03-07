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
#ifndef BOOST_INTRUSIVE_SPLAYTREE_HPP
#define BOOST_INTRUSIVE_SPLAYTREE_HPP

#include <boost/intrusive/detail/config_begin.hpp>
#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>

#include <boost/intrusive/detail/assert.hpp>
#include <boost/static_assert.hpp>
#include <boost/intrusive/intrusive_fwd.hpp>
#include <boost/intrusive/splay_set_hook.hpp>
#include <boost/intrusive/bstree.hpp>
#include <boost/intrusive/detail/tree_node.hpp>
#include <boost/intrusive/detail/ebo_functor_holder.hpp>
#include <boost/intrusive/detail/mpl.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/intrusive/detail/function_detector.hpp>
#include <boost/intrusive/detail/utilities.hpp>
#include <boost/intrusive/options.hpp>
#include <boost/intrusive/splaytree_algorithms.hpp>
#include <boost/intrusive/link_mode.hpp>
#include <boost/move/move.hpp>

namespace boost {
namespace intrusive {

/// @cond

struct splaytree_defaults
{
   typedef detail::default_bstree_hook proto_value_traits;
   static const bool constant_time_size = true;
   typedef std::size_t size_type;
   typedef void compare;
};

/// @endcond

//! The class template splaytree is an intrusive splay tree container that
//! is used to construct intrusive splay_set and splay_multiset containers. The no-throw
//! guarantee holds only, if the value_compare object
//! doesn't throw.
//!
//! The template parameter \c T is the type to be managed by the container.
//! The user can specify additional options and if no options are provided
//! default options are used.
//!
//! The container supports the following options:
//! \c base_hook<>/member_hook<>/value_traits<>,
//! \c constant_time_size<>, \c size_type<> and
//! \c compare<>.
#if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
template<class T, class ...Options>
#else
template<class ValueTraits, class VoidOrKeyComp, class SizeType, bool ConstantTimeSize>
#endif
class splaytree_impl
   /// @cond
   :  public bstree_impl<ValueTraits, VoidOrKeyComp, SizeType, ConstantTimeSize, SplayTreeAlgorithms>
   /// @endcond
{
   public:
   typedef ValueTraits                                               value_traits;
   /// @cond
   typedef bstree_impl< ValueTraits, VoidOrKeyComp, SizeType
                      , ConstantTimeSize, SplayTreeAlgorithms>       tree_type;
   typedef typename tree_type::real_value_traits                     real_value_traits;
   typedef tree_type                                                 implementation_defined;
   /// @endcond

   typedef typename implementation_defined::pointer                  pointer;
   typedef typename implementation_defined::const_pointer            const_pointer;
   typedef typename implementation_defined::value_type               value_type;
   typedef typename implementation_defined::key_type                 key_type;
   typedef typename implementation_defined::reference                reference;
   typedef typename implementation_defined::const_reference          const_reference;
   typedef typename implementation_defined::difference_type          difference_type;
   typedef typename implementation_defined::size_type                size_type;
   typedef typename implementation_defined::value_compare            value_compare;
   typedef typename implementation_defined::key_compare              key_compare;
   typedef typename implementation_defined::iterator                 iterator;
   typedef typename implementation_defined::const_iterator           const_iterator;
   typedef typename implementation_defined::reverse_iterator         reverse_iterator;
   typedef typename implementation_defined::const_reverse_iterator   const_reverse_iterator;
   typedef typename implementation_defined::node_traits              node_traits;
   typedef typename implementation_defined::node                     node;
   typedef typename implementation_defined::node_ptr                 node_ptr;
   typedef typename implementation_defined::const_node_ptr           const_node_ptr;
   typedef typename implementation_defined::node_algorithms          node_algorithms;

   static const bool constant_time_size = implementation_defined::constant_time_size;
   /// @cond
   private:

   //noncopyable
   BOOST_MOVABLE_BUT_NOT_COPYABLE(splaytree_impl)

   /// @endcond

   public:

   typedef typename implementation_defined::insert_commit_data insert_commit_data;

   //! @copydoc ::boost::intrusive::bstree::bstree(const value_compare &,const value_traits &)
   explicit splaytree_impl( const value_compare &cmp = value_compare()
                          , const value_traits &v_traits = value_traits())
      :  tree_type(cmp, v_traits)
   {}

   //! @copydoc ::boost::intrusive::bstree::bstree(bool,Iterator,Iterator,const value_compare &,const value_traits &)
   template<class Iterator>
   splaytree_impl( bool unique, Iterator b, Iterator e
              , const value_compare &cmp     = value_compare()
              , const value_traits &v_traits = value_traits())
      : tree_type(cmp, v_traits)
   {
      if(unique)
         this->insert_unique(b, e);
      else
         this->insert_equal(b, e);
   }

   //! @copydoc ::boost::intrusive::bstree::bstree(bstree &&)
   splaytree_impl(BOOST_RV_REF(splaytree_impl) x)
      :  tree_type(::boost::move(static_cast<tree_type&>(x)))
   {}

   //! @copydoc ::boost::intrusive::bstree::operator=(bstree &&)
   splaytree_impl& operator=(BOOST_RV_REF(splaytree_impl) x)
   {  return static_cast<splaytree_impl&>(tree_type::operator=(::boost::move(static_cast<tree_type&>(x)))); }

   #ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
   //! @copydoc ::boost::intrusive::bstree::~bstree()
   ~splaytree_impl();

   //! @copydoc ::boost::intrusive::bstree::begin()
   iterator begin();

   //! @copydoc ::boost::intrusive::bstree::begin()const
   const_iterator begin() const;

   //! @copydoc ::boost::intrusive::bstree::cbegin()const
   const_iterator cbegin() const;

   //! @copydoc ::boost::intrusive::bstree::end()
   iterator end();

   //! @copydoc ::boost::intrusive::bstree::end()const
   const_iterator end() const;

   //! @copydoc ::boost::intrusive::bstree::cend()const
   const_iterator cend() const;

   //! @copydoc ::boost::intrusive::bstree::rbegin()
   reverse_iterator rbegin();

   //! @copydoc ::boost::intrusive::bstree::rbegin()const
   const_reverse_iterator rbegin() const;

   //! @copydoc ::boost::intrusive::bstree::crbegin()const
   const_reverse_iterator crbegin() const;

   //! @copydoc ::boost::intrusive::bstree::rend()
   reverse_iterator rend();

   //! @copydoc ::boost::intrusive::bstree::rend()const
   const_reverse_iterator rend() const;

   //! @copydoc ::boost::intrusive::bstree::crend()const
   const_reverse_iterator crend() const;

   //! @copydoc ::boost::intrusive::bstree::container_from_end_iterator(iterator)
   static splaytree_impl &container_from_end_iterator(iterator end_iterator);

   //! @copydoc ::boost::intrusive::bstree::container_from_end_iterator(const_iterator)
   static const splaytree_impl &container_from_end_iterator(const_iterator end_iterator);

   //! @copydoc ::boost::intrusive::bstree::container_from_iterator(iterator)
   static splaytree_impl &container_from_iterator(iterator it);

   //! @copydoc ::boost::intrusive::bstree::container_from_iterator(const_iterator)
   static const splaytree_impl &container_from_iterator(const_iterator it);

   //! @copydoc ::boost::intrusive::bstree::key_comp()const
   key_compare key_comp() const;

   //! @copydoc ::boost::intrusive::bstree::value_comp()const
   value_compare value_comp() const;

   //! @copydoc ::boost::intrusive::bstree::empty()const
   bool empty() const;

   //! @copydoc ::boost::intrusive::bstree::size()const
   size_type size() const;

   //! @copydoc ::boost::intrusive::bstree::swap
   void swap(splaytree_impl& other);

   //! @copydoc ::boost::intrusive::bstree::clone_from
   template <class Cloner, class Disposer>
   void clone_from(const splaytree_impl &src, Cloner cloner, Disposer disposer);

   //! @copydoc ::boost::intrusive::bstree::insert_equal(reference)
   iterator insert_equal(reference value);

   //! @copydoc ::boost::intrusive::bstree::insert_equal(const_iterator,reference)
   iterator insert_equal(const_iterator hint, reference value);

   //! @copydoc ::boost::intrusive::bstree::insert_equal(Iterator,Iterator)
   template<class Iterator>
   void insert_equal(Iterator b, Iterator e);

   //! @copydoc ::boost::intrusive::bstree::insert_unique(reference)
   std::pair<iterator, bool> insert_unique(reference value);

   //! @copydoc ::boost::intrusive::bstree::insert_unique(const_iterator,reference)
   iterator insert_unique(const_iterator hint, reference value);

   //! @copydoc ::boost::intrusive::bstree::insert_unique_check(const KeyType&,KeyValueCompare,insert_commit_data&)
   template<class KeyType, class KeyValueCompare>
   std::pair<iterator, bool> insert_unique_check
      (const KeyType &key, KeyValueCompare key_value_comp, insert_commit_data &commit_data);

   //! @copydoc ::boost::intrusive::bstree::insert_unique_check(const_iterator,const KeyType&,KeyValueCompare,insert_commit_data&)
   template<class KeyType, class KeyValueCompare>
   std::pair<iterator, bool> insert_unique_check
      (const_iterator hint, const KeyType &key
      ,KeyValueCompare key_value_comp, insert_commit_data &commit_data);

   //! @copydoc ::boost::intrusive::bstree::insert_unique_commit
   iterator insert_unique_commit(reference value, const insert_commit_data &commit_data);

   //! @copydoc ::boost::intrusive::bstree::insert_unique(Iterator,Iterator)
   template<class Iterator>
   void insert_unique(Iterator b, Iterator e);

   //! @copydoc ::boost::intrusive::bstree::insert_before
   iterator insert_before(const_iterator pos, reference value);

   //! @copydoc ::boost::intrusive::bstree::push_back
   void push_back(reference value);

   //! @copydoc ::boost::intrusive::bstree::push_front
   void push_front(reference value);

   //! @copydoc ::boost::intrusive::bstree::erase(const_iterator)
   iterator erase(const_iterator i);

   //! @copydoc ::boost::intrusive::bstree::erase(const_iterator,const_iterator)
   iterator erase(const_iterator b, const_iterator e);

   //! @copydoc ::boost::intrusive::bstree::erase(const_reference)
   size_type erase(const_reference value);

   //! @copydoc ::boost::intrusive::bstree::erase(const KeyType&,KeyValueCompare)
   template<class KeyType, class KeyValueCompare>

   size_type erase(const KeyType& key, KeyValueCompare comp);

   //! @copydoc ::boost::intrusive::bstree::erase_and_dispose(const_iterator,Disposer)
   template<class Disposer>
   iterator erase_and_dispose(const_iterator i, Disposer disposer);

   //! @copydoc ::boost::intrusive::bstree::erase_and_dispose(const_iterator,const_iterator,Disposer)
   template<class Disposer>
   iterator erase_and_dispose(const_iterator b, const_iterator e, Disposer disposer);

   //! @copydoc ::boost::intrusive::bstree::erase_and_dispose(const_reference, Disposer)
   template<class Disposer>
   size_type erase_and_dispose(const_reference value, Disposer disposer);

   //! @copydoc ::boost::intrusive::bstree::erase_and_dispose(const KeyType&,KeyValueCompare,Disposer)
   template<class KeyType, class KeyValueCompare, class Disposer>
   size_type erase_and_dispose(const KeyType& key, KeyValueCompare comp, Disposer disposer);

   //! @copydoc ::boost::intrusive::bstree::clear
   void clear();

   //! @copydoc ::boost::intrusive::bstree::clear_and_dispose
   template<class Disposer>
   void clear_and_dispose(Disposer disposer);

   #endif   //#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED

   //! @copydoc ::boost::intrusive::bstree::count(const_reference)const
   //! Additional note: non-const function, splaying is performed for the first
   //! element of the equal range of "value"
   size_type count(const_reference value)
   {  return this->count(value, this->value_comp());  }

   //! @copydoc ::boost::intrusive::bstree::count(const KeyType&,KeyValueCompare)const
   //! Additional note: non-const function, splaying is performed for the first
   //! element of the equal range of "key"
   template<class KeyType, class KeyValueCompare>
   size_type count(const KeyType &key, KeyValueCompare comp)
   {
      std::pair<const_iterator, const_iterator> ret = this->equal_range(key, comp);
      return std::distance(ret.first, ret.second);
   }

   //! @copydoc ::boost::intrusive::bstree::count(const_reference)const
   //! Additional note: Deprecated function, use count const overload instead.
   size_type count(const_reference value) const
   {  return tree_type::count(value);  }

   //! @copydoc ::boost::intrusive::bstree::count(const KeyType&,KeyValueCompare)const
   //! Additional note: Deprecated function, use count const overload instead.
   template<class KeyType, class KeyValueCompare>
   size_type count(const KeyType &key, KeyValueCompare comp) const
   {  return tree_type::count(key, comp);  }

   //! @copydoc ::boost::intrusive::bstree::count(const_reference)const
   //! Additional note: Deprecated function, use count const overload instead.
   size_type count_dont_splay(const_reference value) const
   {  return tree_type::count(value);  }

   //! @copydoc ::boost::intrusive::bstree::count(const KeyType&,KeyValueCompare)const
   //! Additional note: Deprecated function, use count const overload instead.
   template<class KeyType, class KeyValueCompare>
   size_type count_dont_splay(const KeyType &key, KeyValueCompare comp) const
   {  return tree_type::count(key, comp);  }

   #ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
   //! @copydoc ::boost::intrusive::bstree::lower_bound(const_reference)
   //! Additional note: non-const function, splaying is performed for the first
   //! element of the equal range of "value"
   iterator lower_bound(const_reference value);

   //! @copydoc ::boost::intrusive::bstree::lower_bound(const_reference)const
   //! Additional note: const function, no splaying is performed
   const_iterator lower_bound(const_reference value) const;

   //! @copydoc ::boost::intrusive::bstree::lower_bound(const_reference)const
   //! Additional note: Deprecated function, use lower_bound const overload instead.
   const_iterator lower_bound_dont_splay(const_reference value) const;

   //! @copydoc ::boost::intrusive::bstree::lower_bound(const KeyType&,KeyValueCompare)
   //! Additional note: non-const function, splaying is performed for the first
   //! element of the equal range of "key"
   template<class KeyType, class KeyValueCompare>
   iterator lower_bound(const KeyType &key, KeyValueCompare comp);

   //! @copydoc ::boost::intrusive::bstree::lower_bound(const KeyType&,KeyValueCompare)const
   //! Additional note: const function, no splaying is performed
   template<class KeyType, class KeyValueCompare>
   const_iterator lower_bound(const KeyType &key, KeyValueCompare comp) const;

   //! @copydoc ::boost::intrusive::bstree::lower_bound(const KeyType&,KeyValueCompare)
   //! Additional note: Deprecated function, use lower_bound const overload instead.
   template<class KeyType, class KeyValueCompare>
   iterator lower_bound_dont_splay(const KeyType &key, KeyValueCompare comp) const;

   //! @copydoc ::boost::intrusive::bstree::upper_bound(const_reference)
   //! Additional note: non-const function, splaying is performed for the first
   //! element of the equal range of "value"
   iterator upper_bound(const_reference value);

   //! @copydoc ::boost::intrusive::bstree::upper_bound(const_reference)const
   //! Additional note: const function, no splaying is performed
   const_iterator upper_bound(const_reference value) const;

   //! @copydoc ::boost::intrusive::bstree::upper_bound(const_reference)const
   //! Additional note: Deprecated function, use upper_bound const overload instead.
   const_iterator upper_bound_dont_splay(const_reference value) const;

   //! @copydoc ::boost::intrusive::bstree::upper_bound(const KeyType&,KeyValueCompare)
   //! Additional note: non-const function, splaying is performed for the first
   //! element of the equal range of "key"
   template<class KeyType, class KeyValueCompare>
   iterator upper_bound(const KeyType &key, KeyValueCompare comp);

   //! @copydoc ::boost::intrusive::bstree::upper_bound(const KeyType&,KeyValueCompare)const
   //! Additional note: const function, no splaying is performed
   template<class KeyType, class KeyValueCompare>
   const_iterator upper_bound(const KeyType &key, KeyValueCompare comp) const;

   //! @copydoc ::boost::intrusive::bstree::upper_bound(const KeyType&,KeyValueCompare)
   //! Additional note: Deprecated function, use upper_bound const overload instead.
   template<class KeyType, class KeyValueCompare>
   const_iterator upper_bound_dont_splay(const KeyType &key, KeyValueCompare comp) const;

   //! @copydoc ::boost::intrusive::bstree::find(const_reference)
   //! Additional note: non-const function, splaying is performed for the first
   //! element of the equal range of "value"
   iterator find(const_reference value);

   //! @copydoc ::boost::intrusive::bstree::find(const_reference)const
   //! Additional note: const function, no splaying is performed
   const_iterator find(const_reference value) const;

   //! @copydoc ::boost::intrusive::bstree::find(const_reference)const
   //! Additional note: Deprecated function, use find const overload instead.
   const_iterator find_dont_splay(const_reference value) const;

   //! @copydoc ::boost::intrusive::bstree::find(const KeyType&,KeyValueCompare)
   //! Additional note: non-const function, splaying is performed for the first
   //! element of the equal range of "key"
   template<class KeyType, class KeyValueCompare>
   iterator find(const KeyType &key, KeyValueCompare comp);

   //! @copydoc ::boost::intrusive::bstree::find(const KeyType&,KeyValueCompare)const
   //! Additional note: const function, no splaying is performed
   template<class KeyType, class KeyValueCompare>
   const_iterator find(const KeyType &key, KeyValueCompare comp) const;

   //! @copydoc ::boost::intrusive::bstree::find(const KeyType&,KeyValueCompare)const
   //! Additional note: Deprecated function, use find const overload instead.
   template<class KeyType, class KeyValueCompare>
   const_iterator find_dont_splay(const KeyType &key, KeyValueCompare comp) const;

   //! @copydoc ::boost::intrusive::bstree::equal_range(const_reference)
   //! Additional note: non-const function, splaying is performed for the first
   //! element of the equal range of "value"
   std::pair<iterator, iterator> equal_range(const_reference value);

   //! @copydoc ::boost::intrusive::bstree::equal_range(const_reference)const
   //! Additional note: const function, no splaying is performed
   std::pair<const_iterator, const_iterator> equal_range(const_reference value) const;

   //! @copydoc ::boost::intrusive::bstree::equal_range(const_reference)const
   //! Additional note: Deprecated function, use equal_range const overload instead.
   std::pair<const_iterator, const_iterator> equal_range_dont_splay(const_reference value) const;

   //! @copydoc ::boost::intrusive::bstree::equal_range(const KeyType&,KeyValueCompare)
   //! Additional note: non-const function, splaying is performed for the first
   //! element of the equal range of "key"
   template<class KeyType, class KeyValueCompare>
   std::pair<iterator, iterator> equal_range(const KeyType &key, KeyValueCompare comp);

   //! @copydoc ::boost::intrusive::bstree::equal_range(const KeyType&,KeyValueCompare)const
   //! Additional note: const function, no splaying is performed
   template<class KeyType, class KeyValueCompare>
   std::pair<const_iterator, const_iterator> equal_range(const KeyType &key, KeyValueCompare comp) const;

   //! @copydoc ::boost::intrusive::bstree::equal_range(const KeyType&,KeyValueCompare)
   //! Additional note: Deprecated function, use equal_range const overload instead.
   template<class KeyType, class KeyValueCompare>
   std::pair<const_iterator, const_iterator> equal_range_dont_splay(const KeyType &key, KeyValueCompare comp) const;

   //! @copydoc ::boost::intrusive::bstree::bounded_range(const_reference,const_reference,bool,bool)
   std::pair<iterator,iterator> bounded_range
      (const_reference lower_value, const_reference upper_value, bool left_closed, bool right_closed);

   //! @copydoc ::boost::intrusive::bstree::bounded_range(const KeyType&,const KeyType&,KeyValueCompare,bool,bool)
   template<class KeyType, class KeyValueCompare>
   std::pair<iterator,iterator> bounded_range
      (const KeyType& lower_key, const KeyType& upper_key, KeyValueCompare comp, bool left_closed, bool right_closed);

   //! @copydoc ::boost::intrusive::bstree::bounded_range(const_reference,const_reference,bool,bool)const
   std::pair<const_iterator, const_iterator> bounded_range
      (const_reference lower_value, const_reference upper_value, bool left_closed, bool right_closed) const;

   //! @copydoc ::boost::intrusive::bstree::bounded_range(const KeyType&,const KeyType&,KeyValueCompare,bool,bool)const
   template<class KeyType, class KeyValueCompare>
   std::pair<const_iterator, const_iterator> bounded_range
         (const KeyType& lower_key, const KeyType& upper_key, KeyValueCompare comp, bool left_closed, bool right_closed) const;

   //! @copydoc ::boost::intrusive::bstree::bounded_range(const_reference,const_reference,bool,bool)const
   //! Additional note: Deprecated function, use bounded_range const overload instead.
   std::pair<const_iterator, const_iterator> bounded_range_dont_splay
      (const_reference lower_value, const_reference upper_value, bool left_closed, bool right_closed) const;

   //! @copydoc ::boost::intrusive::bstree::bounded_range(const KeyType&,const KeyType&,KeyValueCompare,bool,bool)const
   //! Additional note: Deprecated function, use bounded_range const overload instead.
   template<class KeyType, class KeyValueCompare>
   std::pair<const_iterator, const_iterator> bounded_range_dont_splay
      (const KeyType& lower_key, const KeyType& upper_key, KeyValueCompare comp, bool left_closed, bool right_closed) const;

   //! @copydoc ::boost::intrusive::bstree::s_iterator_to(reference)
   static iterator s_iterator_to(reference value);

   //! @copydoc ::boost::intrusive::bstree::s_iterator_to(const_reference)
   static const_iterator s_iterator_to(const_reference value);

   //! @copydoc ::boost::intrusive::bstree::iterator_to(reference)
   iterator iterator_to(reference value);

   //! @copydoc ::boost::intrusive::bstree::iterator_to(const_reference)const
   const_iterator iterator_to(const_reference value) const;

   //! @copydoc ::boost::intrusive::bstree::init_node(reference)
   static void init_node(reference value);

   //! @copydoc ::boost::intrusive::bstree::unlink_leftmost_without_rebalance
   pointer unlink_leftmost_without_rebalance();

   //! @copydoc ::boost::intrusive::bstree::replace_node
   void replace_node(iterator replace_this, reference with_this);

   //! @copydoc ::boost::intrusive::bstree::remove_node
   void remove_node(reference value);

   #endif   //#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED

   //! <b>Requires</b>: i must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Rearranges the container so that the element pointed by i
   //!   is placed as the root of the tree, improving future searches of this value.
   //!
   //! <b>Complexity</b>: Amortized logarithmic.
   //!
   //! <b>Throws</b>: Nothing.
   void splay_up(iterator i)
   {  return node_algorithms::splay_up(i.pointed_node(), tree_type::header_ptr());   }

   //! <b>Effects</b>: Rearranges the container so that if *this stores an element
   //!   with a key equivalent to value the element is placed as the root of the
   //!   tree. If the element is not present returns the last node compared with the key.
   //!   If the tree is empty, end() is returned.
   //!
   //! <b>Complexity</b>: Amortized logarithmic.
   //!
   //! <b>Returns</b>: An iterator to the new root of the tree, end() if the tree is empty.
   //!
   //! <b>Throws</b>: If the comparison functor throws.
   template<class KeyType, class KeyValueCompare>
   iterator splay_down(const KeyType &key, KeyValueCompare comp)
   {
      detail::key_nodeptr_comp<value_compare, real_value_traits>
         key_node_comp(comp, &this->get_real_value_traits());
      node_ptr r = node_algorithms::splay_down(tree_type::header_ptr(), key, key_node_comp);
      return iterator(r, this->real_value_traits_ptr());
   }

   //! <b>Effects</b>: Rearranges the container so that if *this stores an element
   //!   with a key equivalent to value the element is placed as the root of the
   //!   tree.
   //!
   //! <b>Complexity</b>: Amortized logarithmic.
   //!
   //! <b>Returns</b>: An iterator to the new root of the tree, end() if the tree is empty.
   //!
   //! <b>Throws</b>: If the predicate throws.
   iterator splay_down(const_reference value)
   {  return this->splay_down(value, this->value_comp());   }

   #ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
   //! @copydoc ::boost::intrusive::bstree::rebalance
   void rebalance();

   //! @copydoc ::boost::intrusive::bstree::rebalance_subtree
   iterator rebalance_subtree(iterator root);
   #endif   //#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
};

#if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)

template<class T, class ...Options>
bool operator< (const splaytree_impl<T, Options...> &x, const splaytree_impl<T, Options...> &y);

template<class T, class ...Options>
bool operator==(const splaytree_impl<T, Options...> &x, const splaytree_impl<T, Options...> &y);

template<class T, class ...Options>
bool operator!= (const splaytree_impl<T, Options...> &x, const splaytree_impl<T, Options...> &y);

template<class T, class ...Options>
bool operator>(const splaytree_impl<T, Options...> &x, const splaytree_impl<T, Options...> &y);

template<class T, class ...Options>
bool operator<=(const splaytree_impl<T, Options...> &x, const splaytree_impl<T, Options...> &y);

template<class T, class ...Options>
bool operator>=(const splaytree_impl<T, Options...> &x, const splaytree_impl<T, Options...> &y);

template<class T, class ...Options>
void swap(splaytree_impl<T, Options...> &x, splaytree_impl<T, Options...> &y);

#endif   //#if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)

//! Helper metafunction to define a \c splaytree that yields to the same type when the
//! same options (either explicitly or implicitly) are used.
#if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED) || defined(BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
template<class T, class ...Options>
#else
template<class T, class O1 = void, class O2 = void
                , class O3 = void, class O4 = void>
#endif
struct make_splaytree
{
   /// @cond
   typedef typename pack_options
      < splaytree_defaults,
      #if !defined(BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
      O1, O2, O3, O4
      #else
      Options...
      #endif
      >::type packed_options;

   typedef typename detail::get_value_traits
      <T, typename packed_options::proto_value_traits>::type value_traits;

   typedef splaytree_impl
         < value_traits
         , typename packed_options::compare
         , typename packed_options::size_type
         , packed_options::constant_time_size
         > implementation_defined;
   /// @endcond
   typedef implementation_defined type;
};


#ifndef BOOST_INTRUSIVE_DOXYGEN_INVOKED

#if !defined(BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
template<class T, class O1, class O2, class O3, class O4>
#else
template<class T, class ...Options>
#endif
class splaytree
   :  public make_splaytree<T,
      #if !defined(BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
      O1, O2, O3, O4
      #else
      Options...
      #endif
      >::type
{
   typedef typename make_splaytree
      <T,
      #if !defined(BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
      O1, O2, O3, O4
      #else
      Options...
      #endif
      >::type   Base;
   BOOST_MOVABLE_BUT_NOT_COPYABLE(splaytree)

   public:
   typedef typename Base::value_compare      value_compare;
   typedef typename Base::value_traits       value_traits;
   typedef typename Base::real_value_traits  real_value_traits;
   typedef typename Base::iterator           iterator;
   typedef typename Base::const_iterator     const_iterator;
   typedef typename Base::reverse_iterator           reverse_iterator;
   typedef typename Base::const_reverse_iterator     const_reverse_iterator;

   //Assert if passed value traits are compatible with the type
   BOOST_STATIC_ASSERT((detail::is_same<typename real_value_traits::value_type, T>::value));

   explicit splaytree( const value_compare &cmp = value_compare()
                     , const value_traits &v_traits = value_traits())
      :  Base(cmp, v_traits)
   {}

   template<class Iterator>
   splaytree( bool unique, Iterator b, Iterator e
         , const value_compare &cmp = value_compare()
         , const value_traits &v_traits = value_traits())
      :  Base(unique, b, e, cmp, v_traits)
   {}

   splaytree(BOOST_RV_REF(splaytree) x)
      :  Base(::boost::move(static_cast<Base&>(x)))
   {}

   splaytree& operator=(BOOST_RV_REF(splaytree) x)
   {  return static_cast<splaytree &>(this->Base::operator=(::boost::move(static_cast<Base&>(x))));  }

   static splaytree &container_from_end_iterator(iterator end_iterator)
   {  return static_cast<splaytree &>(Base::container_from_end_iterator(end_iterator));   }

   static const splaytree &container_from_end_iterator(const_iterator end_iterator)
   {  return static_cast<const splaytree &>(Base::container_from_end_iterator(end_iterator));   }

   static splaytree &container_from_iterator(iterator it)
   {  return static_cast<splaytree &>(Base::container_from_iterator(it));   }

   static const splaytree &container_from_iterator(const_iterator it)
   {  return static_cast<const splaytree &>(Base::container_from_iterator(it));   }
};

#endif

} //namespace intrusive
} //namespace boost

#include <boost/intrusive/detail/config_end.hpp>

#endif //BOOST_INTRUSIVE_SPLAYTREE_HPP
