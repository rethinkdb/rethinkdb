/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2013-2013
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
#include <boost/intrusive/avl_set.hpp>
#include <boost/intrusive/sg_set.hpp>
#include <boost/intrusive/bs_set.hpp>
#include <boost/intrusive/splay_set.hpp>
#include <boost/intrusive/treap_set.hpp>
#include <boost/intrusive/detail/mpl.hpp>
#include <boost/static_assert.hpp>
#include "smart_ptr.hpp"
#include <functional>   //std::greater/std::less

using namespace boost::intrusive;
struct my_tag;

template<class VoidPtr = void*, link_mode_type mode = normal_link>
class MyClass
:  public make_list_base_hook
   < void_pointer<VoidPtr>, link_mode<mode> >::type
,  public make_slist_base_hook
   < void_pointer<VoidPtr>, link_mode<mode> >::type
,  public make_set_base_hook
   < void_pointer<VoidPtr>, link_mode<mode> >::type
,  public make_unordered_set_base_hook
   < void_pointer<VoidPtr>, link_mode<mode> >::type
,  public make_avl_set_base_hook
   < void_pointer<VoidPtr>, link_mode<mode> >::type
,  public make_bs_set_base_hook
   < void_pointer<VoidPtr>, link_mode<mode> >::type
{
   int int_;

   public:
   MyClass(int i)
      :  int_(i)
   {}

   friend bool operator<(const MyClass &l, const MyClass &r)
   {  return l.int_ < r.int_; }

   friend bool operator==(const MyClass &l, const MyClass &r)
   {  return l.int_ == r.int_; }

   friend std::size_t hash_value(const MyClass &v)
   {  return boost::hash_value(v.int_); }

   friend bool priority_order(const MyClass &l, const MyClass &r)
   {  return l.int_ < r.int_; }
};

template<class T>
struct inverse_priority
{
   bool operator()(const T &l, const T &r)
   {  return l.int_ > r.int_; }
};

template<class T>
struct inverse_hash
{
   bool operator()(const T &l)
   {  return hash_value(l); }
};

template<class T>
struct alternative_equal
{
   bool operator()(const T &l, const T &r)
   {  return l.int_ == r.int_; }
};

int main()
{
   ////////////
   // list
   ////////////
   BOOST_STATIC_ASSERT((!detail::is_same< list<MyClass<> >::iterator
                                        , list<MyClass<> >::const_iterator
                         >::value));
   //constant_time_size does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< list<MyClass<>, constant_time_size<true>  >::iterator
                                       , list<MyClass<>, constant_time_size<false> >::iterator
                        >::value));
   //void_pointer does change iterator
   BOOST_STATIC_ASSERT((!detail::is_same< list<MyClass<> >::iterator
                                        , list<MyClass<smart_ptr<void> > >::iterator
                         >::value));
   //size_type does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< list<MyClass<>, size_type<unsigned int > >::iterator
                                       , list<MyClass<>, size_type<unsigned char> >::iterator
                         >::value));
   ////////////
   // slist
   ////////////
   BOOST_STATIC_ASSERT((!detail::is_same< slist<MyClass<> >::iterator
                                        , slist<MyClass<> >::const_iterator
                         >::value));
   //constant_time_size does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< slist<MyClass<>, constant_time_size<true>  >::iterator
                                       , slist<MyClass<>, constant_time_size<false> >::iterator
                        >::value));
   //void_pointer does change iterator
   BOOST_STATIC_ASSERT((!detail::is_same< slist<MyClass<> >::iterator
                                        , slist<MyClass<smart_ptr<void> > >::iterator
                         >::value));
   //size_type does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< slist<MyClass<>, size_type<unsigned int > >::iterator
                                       , slist<MyClass<>, size_type<unsigned char> >::iterator
                         >::value));
   //cache_last does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< slist<MyClass<>, cache_last<false> >::iterator
                                       , slist<MyClass<>, cache_last<true>  >::iterator
                         >::value));
   //linear does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< slist<MyClass<>, linear<false> >::iterator
                                       , slist<MyClass<>, linear<true>  >::iterator
                         >::value));
   ////////////
   // set
   ////////////
   BOOST_STATIC_ASSERT((!detail::is_same< set<MyClass<> >::iterator
                                        , set<MyClass<> >::const_iterator
                         >::value));
   //constant_time_size does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< set<MyClass<>, constant_time_size<true>  >::iterator
                                       , set<MyClass<>, constant_time_size<false> >::iterator
                        >::value));
   //void_pointer does change iterator
   BOOST_STATIC_ASSERT((!detail::is_same< set<MyClass<> >::iterator
                                        , set<MyClass<smart_ptr<void> > >::iterator
                         >::value));
   //size_type does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< set<MyClass<>, size_type<unsigned int > >::iterator
                                       , set<MyClass<>, size_type<unsigned char> >::iterator
                         >::value));
   //compare does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< set<MyClass<>, compare< std::greater<MyClass<> > > >::iterator
                                       , set<MyClass<>, compare< std::less<MyClass<> > > >::iterator
                         >::value));
   ////////////
   // avl_set
   ////////////
   BOOST_STATIC_ASSERT((!detail::is_same< avl_set<MyClass<> >::iterator
                                        , avl_set<MyClass<> >::const_iterator
                         >::value));
   //constant_time_size does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< avl_set<MyClass<>, constant_time_size<true>  >::iterator
                                       , avl_set<MyClass<>, constant_time_size<false> >::iterator
                        >::value));
   //void_pointer does change iterator
   BOOST_STATIC_ASSERT((!detail::is_same< avl_set<MyClass<> >::iterator
                                        , avl_set<MyClass<smart_ptr<void> > >::iterator
                         >::value));
   //size_type does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< avl_set<MyClass<>, size_type<unsigned int > >::iterator
                                       , avl_set<MyClass<>, size_type<unsigned char> >::iterator
                         >::value));
   //compare does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< avl_set<MyClass<>, compare< std::greater<MyClass<> > > >::iterator
                                       , avl_set<MyClass<>, compare< std::less<MyClass<> > > >::iterator
                         >::value));
   ////////////
   // sg_set
   ////////////
   BOOST_STATIC_ASSERT((!detail::is_same< sg_set<MyClass<> >::iterator
                                        , sg_set<MyClass<> >::const_iterator
                         >::value));
   //void_pointer does change iterator
   BOOST_STATIC_ASSERT((!detail::is_same< sg_set<MyClass<> >::iterator
                                        , sg_set<MyClass<smart_ptr<void> > >::iterator
                         >::value));
   //size_type does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< sg_set<MyClass<>, size_type<unsigned int > >::iterator
                                       , sg_set<MyClass<>, size_type<unsigned char> >::iterator
                         >::value));
   //compare does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< sg_set<MyClass<>, compare< std::greater<MyClass<> > > >::iterator
                                       , sg_set<MyClass<>, compare< std::less<MyClass<> > > >::iterator
                         >::value));
   //floating_point does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< sg_set<MyClass<>, floating_point<false> >::iterator
                                       , sg_set<MyClass<>, floating_point<true>  >::iterator
                         >::value));
   ////////////
   // bs_set
   ////////////
   BOOST_STATIC_ASSERT((!detail::is_same< bs_set<MyClass<> >::iterator
                                        , bs_set<MyClass<> >::const_iterator
                         >::value));
   //constant_time_size does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< bs_set<MyClass<>, constant_time_size<true>  >::iterator
                                       , bs_set<MyClass<>, constant_time_size<false> >::iterator
                         >::value));
   //void_pointer does change iterator
   BOOST_STATIC_ASSERT((!detail::is_same< bs_set<MyClass<> >::iterator
                                        , bs_set<MyClass<smart_ptr<void> > >::iterator
                         >::value));
   //size_type does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< bs_set<MyClass<>, size_type<unsigned int > >::iterator
                                       , bs_set<MyClass<>, size_type<unsigned char> >::iterator
                         >::value));
   //compare does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< bs_set<MyClass<>, compare< std::greater<MyClass<> > > >::iterator
                                       , bs_set<MyClass<>, compare< std::less<MyClass<> > > >::iterator
                         >::value));
   ////////////
   // splay_set
   ////////////
   BOOST_STATIC_ASSERT((!detail::is_same< splay_set<MyClass<> >::iterator
                                        , splay_set<MyClass<> >::const_iterator
                         >::value));
   //constant_time_size does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< splay_set<MyClass<>, constant_time_size<true>  >::iterator
                                       , splay_set<MyClass<>, constant_time_size<false> >::iterator
                        >::value));
   //void_pointer does change iterator
   BOOST_STATIC_ASSERT((!detail::is_same< splay_set<MyClass<> >::iterator
                                        , splay_set<MyClass<smart_ptr<void> > >::iterator
                         >::value));
   //size_type does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< splay_set<MyClass<>, size_type<unsigned int > >::iterator
                                       , splay_set<MyClass<>, size_type<unsigned char> >::iterator
                         >::value));
   //compare does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< splay_set<MyClass<>, compare< std::greater<MyClass<> > > >::iterator
                                       , splay_set<MyClass<>, compare< std::less<MyClass<> > > >::iterator
                         >::value));
   ////////////
   // treap_set
   ////////////
   BOOST_STATIC_ASSERT((!detail::is_same< treap_set<MyClass<> >::iterator
                                        , treap_set<MyClass<> >::const_iterator
                         >::value));
   //constant_time_size does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< treap_set<MyClass<>, constant_time_size<true>  >::iterator
                                       , treap_set<MyClass<>, constant_time_size<false> >::iterator
                        >::value));
   //void_pointer does change iterator
   BOOST_STATIC_ASSERT((!detail::is_same< treap_set<MyClass<> >::iterator
                                        , treap_set<MyClass<smart_ptr<void> > >::iterator
                         >::value));
   //size_type does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< treap_set<MyClass<>, size_type<unsigned int > >::iterator
                                       , treap_set<MyClass<>, size_type<unsigned char> >::iterator
                         >::value));
   //compare does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< treap_set<MyClass<>, compare< std::greater<MyClass<> > > >::iterator
                                       , treap_set<MyClass<>, compare< std::less<MyClass<> > > >::iterator
                         >::value));
   //priority does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< treap_set<MyClass<> >::iterator
                                       , treap_set<MyClass<>, priority< inverse_priority<MyClass<> > > >::iterator
                         >::value));
   //////////////
   // common tree
   //////////////
   BOOST_STATIC_ASSERT((detail::is_same< bs_set<MyClass<> >::iterator
                                       , sg_set<MyClass<> >::iterator
                         >::value));
   BOOST_STATIC_ASSERT((detail::is_same< bs_set<MyClass<> >::iterator
                                       , treap_set<MyClass<> >::iterator
                         >::value));
   BOOST_STATIC_ASSERT((detail::is_same< bs_set<MyClass<> >::iterator
                                       , splay_set<MyClass<> >::iterator
                         >::value));
   ////////////
   // unordered_set
   ////////////
   BOOST_STATIC_ASSERT((!detail::is_same< unordered_set<MyClass<> >::iterator
                                        , unordered_set<MyClass<> >::const_iterator
                         >::value));
   //constant_time_size does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< unordered_set<MyClass<>, constant_time_size<true>  >::iterator
                                       , unordered_set<MyClass<>, constant_time_size<false> >::iterator
                        >::value));
   //void_pointer does change iterator
   BOOST_STATIC_ASSERT((!detail::is_same< unordered_set<MyClass<> >::iterator
                                        , unordered_set<MyClass<smart_ptr<void> > >::iterator
                         >::value));
   //size_type does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< unordered_set<MyClass<>, size_type<unsigned int > >::iterator
                                       , unordered_set<MyClass<>, size_type<unsigned char> >::iterator
                         >::value));
   //hash does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< unordered_set<MyClass<> >::iterator
                                       , unordered_set<MyClass<>, hash< inverse_hash<MyClass<> > > >::iterator
                         >::value));
   //equal does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< unordered_set<MyClass<> >::iterator
                                       , unordered_set<MyClass<>, equal< alternative_equal<MyClass<> > > >::iterator
                         >::value));
   //power_2_buckets does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< unordered_set<MyClass<> >::iterator
                                       , unordered_set<MyClass<>, power_2_buckets<true> >::iterator
                         >::value));
   //cache_begin does not change iterator
   BOOST_STATIC_ASSERT((detail::is_same< unordered_set<MyClass<> >::iterator
                                       , unordered_set<MyClass<>, cache_begin<true> >::iterator
                         >::value));
   return 0;
}
