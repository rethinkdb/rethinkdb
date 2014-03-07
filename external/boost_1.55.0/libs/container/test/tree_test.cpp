//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/container/detail/config_begin.hpp>
#include <set>
#include <boost/container/set.hpp>
#include <boost/container/map.hpp>
#include "print_container.hpp"
#include "movable_int.hpp"
#include "dummy_test_allocator.hpp"
#include "set_test.hpp"
#include "map_test.hpp"
#include "propagate_allocator_test.hpp"
#include "emplace_test.hpp"

using namespace boost::container;

//Alias standard types
typedef std::set<int>                                          MyStdSet;
typedef std::multiset<int>                                     MyStdMultiSet;
typedef std::map<int, int>                                     MyStdMap;
typedef std::multimap<int, int>                                MyStdMultiMap;

//Alias non-movable types
typedef set<int>           MyBoostSet;
typedef multiset<int>      MyBoostMultiSet;
typedef map<int, int>      MyBoostMap;
typedef multimap<int, int> MyBoostMultiMap;

//Alias movable types
typedef set<test::movable_int>                           MyMovableBoostSet;
typedef multiset<test::movable_int>                      MyMovableBoostMultiSet;
typedef map<test::movable_int, test::movable_int>        MyMovableBoostMap;
typedef multimap<test::movable_int, test::movable_int>   MyMovableBoostMultiMap;
typedef set<test::movable_and_copyable_int>              MyMoveCopyBoostSet;
typedef set<test::copyable_int>                          MyCopyBoostSet;
typedef multiset<test::movable_and_copyable_int>         MyMoveCopyBoostMultiSet;
typedef multiset<test::copyable_int>                     MyCopyBoostMultiSet;
typedef map<test::movable_and_copyable_int
           ,test::movable_and_copyable_int>              MyMoveCopyBoostMap;
typedef multimap<test::movable_and_copyable_int
                ,test::movable_and_copyable_int>         MyMoveCopyBoostMultiMap;
typedef map<test::copyable_int
           ,test::copyable_int>                          MyCopyBoostMap;
typedef multimap<test::copyable_int
                ,test::copyable_int>                     MyCopyBoostMultiMap;

namespace boost {
namespace container {

//Explicit instantiation to detect compilation errors

//map
template class map
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::dummy_test_allocator
      < std::pair<const test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class map
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::simple_allocator
      < std::pair<const test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class map
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , std::allocator
      < std::pair<const test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

//multimap
template class multimap
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::dummy_test_allocator
      < std::pair<const test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class multimap
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::simple_allocator
      < std::pair<const test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class multimap
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , std::allocator
      < std::pair<const test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

//set
template class set
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::dummy_test_allocator<test::movable_and_copyable_int>
   >;

template class set
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::simple_allocator<test::movable_and_copyable_int>
   >;

template class set
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , std::allocator<test::movable_and_copyable_int>
   >;

//multiset
template class multiset
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::dummy_test_allocator<test::movable_and_copyable_int>
   >;

template class multiset
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::simple_allocator<test::movable_and_copyable_int>
   >;

template class multiset
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , std::allocator<test::movable_and_copyable_int>
   >;

}} //boost::container

//Test recursive structures
class recursive_set
{
public:
   recursive_set & operator=(const recursive_set &x)
   {  id_ = x.id_;  set_ = x.set_; return *this; }

   int id_;
   set<recursive_set> set_;
   friend bool operator< (const recursive_set &a, const recursive_set &b)
   {  return a.id_ < b.id_;   }
};

class recursive_map
{
   public:
   recursive_map & operator=(const recursive_map &x)
   {  id_ = x.id_;  map_ = x.map_; return *this;  }

   int id_;
   map<recursive_map, recursive_map> map_;
   friend bool operator< (const recursive_map &a, const recursive_map &b)
   {  return a.id_ < b.id_;   }
};

//Test recursive structures
class recursive_multiset
{
   public:
   recursive_multiset & operator=(const recursive_multiset &x)
   {  id_ = x.id_;  multiset_ = x.multiset_; return *this;  }

   int id_;
   multiset<recursive_multiset> multiset_;
   friend bool operator< (const recursive_multiset &a, const recursive_multiset &b)
   {  return a.id_ < b.id_;   }
};

class recursive_multimap
{
   public:
   recursive_multimap & operator=(const recursive_multimap &x)
   {  id_ = x.id_;  multimap_ = x.multimap_; return *this;  }

   int id_;
   multimap<recursive_multimap, recursive_multimap> multimap_;
   friend bool operator< (const recursive_multimap &a, const recursive_multimap &b)
   {  return a.id_ < b.id_;   }
};

template<class C>
void test_move()
{
   //Now test move semantics
   C original;
   original.emplace();
   C move_ctor(boost::move(original));
   C move_assign;
   move_assign.emplace();
   move_assign = boost::move(move_ctor);
   move_assign.swap(original);
}

template<class T, class A>
class tree_propagate_test_wrapper
   : public container_detail::rbtree<T, T, container_detail::identity<T>, std::less<T>, A>
{
   BOOST_COPYABLE_AND_MOVABLE(tree_propagate_test_wrapper)
   typedef container_detail::rbtree<T, T, container_detail::identity<T>, std::less<T>, A> Base;
   public:
   tree_propagate_test_wrapper()
      : Base()
   {}

   tree_propagate_test_wrapper(const tree_propagate_test_wrapper &x)
      : Base(x)
   {}

   tree_propagate_test_wrapper(BOOST_RV_REF(tree_propagate_test_wrapper) x)
      : Base(boost::move(static_cast<Base&>(x)))
   {}

   tree_propagate_test_wrapper &operator=(BOOST_COPY_ASSIGN_REF(tree_propagate_test_wrapper) x)
   {  this->Base::operator=(x);  return *this; }

   tree_propagate_test_wrapper &operator=(BOOST_RV_REF(tree_propagate_test_wrapper) x)
   {  this->Base::operator=(boost::move(static_cast<Base&>(x)));  return *this; }

   void swap(tree_propagate_test_wrapper &x)
   {  this->Base::swap(x);  }
};

int main ()
{
   //Recursive container instantiation
   {
      set<recursive_set> set_;
      multiset<recursive_multiset> multiset_;
      map<recursive_map, recursive_map> map_;
      multimap<recursive_multimap, recursive_multimap> multimap_;
   }
   //Allocator argument container
   {
      set<int> set_((std::allocator<int>()));
      multiset<int> multiset_((std::allocator<int>()));
      map<int, int> map_((std::allocator<std::pair<const int, int> >()));
      multimap<int, int> multimap_((std::allocator<std::pair<const int, int> >()));
   }
   //Now test move semantics
   {
      test_move<set<recursive_set> >();
      test_move<multiset<recursive_multiset> >();
      test_move<map<recursive_map, recursive_map> >();
      test_move<multimap<recursive_multimap, recursive_multimap> >();
   }


   if(0 != test::set_test<MyBoostSet
                        ,MyStdSet
                        ,MyBoostMultiSet
                        ,MyStdMultiSet>()){
      return 1;
   }

   if(0 != test::set_test_copyable<MyBoostSet
                        ,MyStdSet
                        ,MyBoostMultiSet
                        ,MyStdMultiSet>()){
      return 1;
   }

   if(0 != test::set_test<MyMovableBoostSet
                        ,MyStdSet
                        ,MyMovableBoostMultiSet
                        ,MyStdMultiSet>()){
      return 1;
   }

   if(0 != test::set_test<MyMoveCopyBoostSet
                        ,MyStdSet
                        ,MyMoveCopyBoostMultiSet
                        ,MyStdMultiSet>()){
      return 1;
   }

   if(0 != test::set_test_copyable<MyMoveCopyBoostSet
                        ,MyStdSet
                        ,MyMoveCopyBoostMultiSet
                        ,MyStdMultiSet>()){
      return 1;
   }

   if(0 != test::set_test<MyCopyBoostSet
                        ,MyStdSet
                        ,MyCopyBoostMultiSet
                        ,MyStdMultiSet>()){
      return 1;
   }

   if(0 != test::set_test_copyable<MyCopyBoostSet
                        ,MyStdSet
                        ,MyCopyBoostMultiSet
                        ,MyStdMultiSet>()){
      return 1;
   }

   if (0 != test::map_test<MyBoostMap
                  ,MyStdMap
                  ,MyBoostMultiMap
                  ,MyStdMultiMap>()){
      return 1;
   }

   if(0 != test::map_test_copyable<MyBoostMap
                        ,MyStdMap
                        ,MyBoostMultiMap
                        ,MyStdMultiMap>()){
      return 1;
   }

   if (0 != test::map_test<MyMovableBoostMap
                  ,MyStdMap
                  ,MyMovableBoostMultiMap
                  ,MyStdMultiMap>()){
      return 1;
   }

   if (0 != test::map_test<MyMoveCopyBoostMap
                  ,MyStdMap
                  ,MyMoveCopyBoostMultiMap
                  ,MyStdMultiMap>()){
      return 1;
   }

   if (0 != test::map_test_copyable<MyMoveCopyBoostMap
                  ,MyStdMap
                  ,MyMoveCopyBoostMultiMap
                  ,MyStdMultiMap>()){
      return 1;
   }

   if (0 != test::map_test<MyCopyBoostMap
                  ,MyStdMap
                  ,MyCopyBoostMultiMap
                  ,MyStdMultiMap>()){
      return 1;
   }

   if (0 != test::map_test_copyable<MyCopyBoostMap
                  ,MyStdMap
                  ,MyCopyBoostMultiMap
                  ,MyStdMultiMap>()){
      return 1;
   }

   const test::EmplaceOptions SetOptions = (test::EmplaceOptions)(test::EMPLACE_HINT | test::EMPLACE_ASSOC);
   if(!boost::container::test::test_emplace<set<test::EmplaceInt>, SetOptions>())
      return 1;
   if(!boost::container::test::test_emplace<multiset<test::EmplaceInt>, SetOptions>())
      return 1;
   const test::EmplaceOptions MapOptions = (test::EmplaceOptions)(test::EMPLACE_HINT_PAIR | test::EMPLACE_ASSOC_PAIR);
   if(!boost::container::test::test_emplace<map<test::EmplaceInt, test::EmplaceInt>, MapOptions>())
      return 1;
   if(!boost::container::test::test_emplace<multimap<test::EmplaceInt, test::EmplaceInt>, MapOptions>())
      return 1;
   if(!boost::container::test::test_propagate_allocator<tree_propagate_test_wrapper>())
      return 1;

   return 0;
}

#include <boost/container/detail/config_end.hpp>
