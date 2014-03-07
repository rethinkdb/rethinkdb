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
#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>
#include "print_container.hpp"
#include "dummy_test_allocator.hpp"
#include "movable_int.hpp"
#include "set_test.hpp"
#include "map_test.hpp"
#include "propagate_allocator_test.hpp"
#include "emplace_test.hpp"
#include <vector>
#include <boost/container/detail/flat_tree.hpp>

using namespace boost::container;

namespace boost {
namespace container {

//Explicit instantiation to detect compilation errors

//flat_map
template class flat_map
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::dummy_test_allocator
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class flat_map
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::simple_allocator
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class flat_map
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , std::allocator
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

//flat_multimap
template class flat_multimap
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::dummy_test_allocator
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class flat_multimap
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::simple_allocator
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;

template class flat_multimap
   < test::movable_and_copyable_int
   , test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , std::allocator
      < std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   >;
//flat_set
template class flat_set
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::dummy_test_allocator<test::movable_and_copyable_int>
   >;

template class flat_set
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::simple_allocator<test::movable_and_copyable_int>
   >;

template class flat_set
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , std::allocator<test::movable_and_copyable_int>
   >;

//flat_multiset
template class flat_multiset
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::dummy_test_allocator<test::movable_and_copyable_int>
   >;

template class flat_multiset
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , test::simple_allocator<test::movable_and_copyable_int>
   >;

template class flat_multiset
   < test::movable_and_copyable_int
   , std::less<test::movable_and_copyable_int>
   , std::allocator<test::movable_and_copyable_int>
   >;

//As flat container iterators are typedefs for vector::[const_]iterator,
//no need to explicit instantiate them

}} //boost::container


//Alias allocator type
typedef std::allocator<int> allocator_t;
typedef std::allocator<test::movable_int>
   movable_allocator_t;
typedef std::allocator<std::pair<int, int> >
   pair_allocator_t;
typedef std::allocator<std::pair<test::movable_int, test::movable_int> >
   movable_pair_allocator_t;
typedef std::allocator<test::movable_and_copyable_int >
   move_copy_allocator_t;
typedef std::allocator<std::pair<test::movable_and_copyable_int, test::movable_and_copyable_int> >
   move_copy_pair_allocator_t;
typedef std::allocator<test::copyable_int >
   copy_allocator_t;
typedef std::allocator<std::pair<test::copyable_int, test::copyable_int> >
   copy_pair_allocator_t;


//Alias set types
typedef std::set<int>                                                   MyStdSet;
typedef std::multiset<int>                                              MyStdMultiSet;
typedef std::map<int, int>                                              MyStdMap;
typedef std::multimap<int, int>                                         MyStdMultiMap;

typedef flat_set<int, std::less<int>, allocator_t>                MyBoostSet;
typedef flat_multiset<int, std::less<int>, allocator_t>           MyBoostMultiSet;
typedef flat_map<int, int, std::less<int>, pair_allocator_t>      MyBoostMap;
typedef flat_multimap<int, int, std::less<int>, pair_allocator_t> MyBoostMultiMap;

typedef flat_set<test::movable_int, std::less<test::movable_int>
                ,movable_allocator_t>                             MyMovableBoostSet;
typedef flat_multiset<test::movable_int,std::less<test::movable_int>
                     ,movable_allocator_t>                        MyMovableBoostMultiSet;
typedef flat_map<test::movable_int, test::movable_int
                ,std::less<test::movable_int>
                ,movable_pair_allocator_t>                        MyMovableBoostMap;
typedef flat_multimap<test::movable_int, test::movable_int
                ,std::less<test::movable_int>
                ,movable_pair_allocator_t>                        MyMovableBoostMultiMap;

typedef flat_set<test::movable_and_copyable_int, std::less<test::movable_and_copyable_int>
                ,move_copy_allocator_t>                             MyMoveCopyBoostSet;
typedef flat_multiset<test::movable_and_copyable_int,std::less<test::movable_and_copyable_int>
                     ,move_copy_allocator_t>                        MyMoveCopyBoostMultiSet;
typedef flat_map<test::movable_and_copyable_int, test::movable_and_copyable_int
                ,std::less<test::movable_and_copyable_int>
                ,move_copy_pair_allocator_t>                        MyMoveCopyBoostMap;
typedef flat_multimap<test::movable_and_copyable_int, test::movable_and_copyable_int
                ,std::less<test::movable_and_copyable_int>
                ,move_copy_pair_allocator_t>                        MyMoveCopyBoostMultiMap;

typedef flat_set<test::copyable_int, std::less<test::copyable_int>
                ,copy_allocator_t>                             MyCopyBoostSet;
typedef flat_multiset<test::copyable_int,std::less<test::copyable_int>
                     ,copy_allocator_t>                        MyCopyBoostMultiSet;
typedef flat_map<test::copyable_int, test::copyable_int
                ,std::less<test::copyable_int>
                ,copy_pair_allocator_t>                        MyCopyBoostMap;
typedef flat_multimap<test::copyable_int, test::copyable_int
                ,std::less<test::copyable_int>
                ,copy_pair_allocator_t>                        MyCopyBoostMultiMap;


//Test recursive structures
class recursive_flat_set
{
   public:
   recursive_flat_set(const recursive_flat_set &c)
      : id_(c.id_), flat_set_(c.flat_set_)
   {}

   recursive_flat_set & operator =(const recursive_flat_set &c)
   {
      id_ = c.id_;
      flat_set_= c.flat_set_;
      return *this;
   }
   int id_;
   flat_set<recursive_flat_set> flat_set_;
   friend bool operator< (const recursive_flat_set &a, const recursive_flat_set &b)
   {  return a.id_ < b.id_;   }
};



class recursive_flat_map
{
   public:
   recursive_flat_map(const recursive_flat_map &c)
      : id_(c.id_), map_(c.map_)
   {}

   recursive_flat_map & operator =(const recursive_flat_map &c)
   {
      id_ = c.id_;
      map_= c.map_;
      return *this;
   }

   int id_;
   flat_map<recursive_flat_map, recursive_flat_map> map_;

   friend bool operator< (const recursive_flat_map &a, const recursive_flat_map &b)
   {  return a.id_ < b.id_;   }
};

//Test recursive structures
class recursive_flat_multiset
{
   public:
   recursive_flat_multiset(const recursive_flat_multiset &c)
      : id_(c.id_), flat_multiset_(c.flat_multiset_)
   {}

   recursive_flat_multiset & operator =(const recursive_flat_multiset &c)
   {
      id_ = c.id_;
      flat_multiset_= c.flat_multiset_;
      return *this;
   }
   int id_;
   flat_multiset<recursive_flat_multiset> flat_multiset_;
   friend bool operator< (const recursive_flat_multiset &a, const recursive_flat_multiset &b)
   {  return a.id_ < b.id_;   }
};

class recursive_flat_multimap
{
public:
   recursive_flat_multimap(const recursive_flat_multimap &c)
      : id_(c.id_), map_(c.map_)
   {}

   recursive_flat_multimap & operator =(const recursive_flat_multimap &c)
   {
      id_ = c.id_;
      map_= c.map_;
      return *this;
   }
   int id_;
   flat_map<recursive_flat_multimap, recursive_flat_multimap> map_;
   friend bool operator< (const recursive_flat_multimap &a, const recursive_flat_multimap &b)
   {  return a.id_ < b.id_;   }
};

template<class C>
void test_move()
{
   //Now test move semantics
   C original;
   C move_ctor(boost::move(original));
   C move_assign;
   move_assign = boost::move(move_ctor);
   move_assign.swap(original);
}

template<class T, class A>
class flat_tree_propagate_test_wrapper
   : public container_detail::flat_tree<T, T, container_detail::identity<T>, std::less<T>, A>
{
   BOOST_COPYABLE_AND_MOVABLE(flat_tree_propagate_test_wrapper)
   typedef container_detail::flat_tree<T, T, container_detail::identity<T>, std::less<T>, A> Base;
   public:
   flat_tree_propagate_test_wrapper()
      : Base()
   {}

   flat_tree_propagate_test_wrapper(const flat_tree_propagate_test_wrapper &x)
      : Base(x)
   {}

   flat_tree_propagate_test_wrapper(BOOST_RV_REF(flat_tree_propagate_test_wrapper) x)
      : Base(boost::move(static_cast<Base&>(x)))
   {}

   flat_tree_propagate_test_wrapper &operator=(BOOST_COPY_ASSIGN_REF(flat_tree_propagate_test_wrapper) x)
   {  this->Base::operator=(x);  return *this; }

   flat_tree_propagate_test_wrapper &operator=(BOOST_RV_REF(flat_tree_propagate_test_wrapper) x)
   {  this->Base::operator=(boost::move(static_cast<Base&>(x)));  return *this; }

   void swap(flat_tree_propagate_test_wrapper &x)
   {  this->Base::swap(x);  }
};

namespace boost{
namespace container {
namespace test{

bool flat_tree_ordered_insertion_test()
{
   using namespace boost::container;
   const std::size_t NumElements = 100;

   //Ordered insertion multiset
   {
      std::multiset<int> int_mset;
      for(std::size_t i = 0; i != NumElements; ++i){
         int_mset.insert(static_cast<int>(i));
      }
      //Construction insertion
      flat_multiset<int> fmset(ordered_range, int_mset.begin(), int_mset.end());
      if(!CheckEqualContainers(&int_mset, &fmset))
         return false;
      //Insertion when empty
      fmset.clear();
      fmset.insert(ordered_range, int_mset.begin(), int_mset.end());
      if(!CheckEqualContainers(&int_mset, &fmset))
         return false;
      //Re-insertion
      fmset.insert(ordered_range, int_mset.begin(), int_mset.end());
      std::multiset<int> int_mset2(int_mset);
      int_mset2.insert(int_mset.begin(), int_mset.end());
      if(!CheckEqualContainers(&int_mset2, &fmset))
         return false;
      //Re-re-insertion
      fmset.insert(ordered_range, int_mset2.begin(), int_mset2.end());
      std::multiset<int> int_mset4(int_mset2);
      int_mset4.insert(int_mset2.begin(), int_mset2.end());
      if(!CheckEqualContainers(&int_mset4, &fmset))
         return false;
      //Re-re-insertion of even
      std::multiset<int> int_even_mset;
      for(std::size_t i = 0; i < NumElements; i+=2){
         int_mset.insert(static_cast<int>(i));
      }
      fmset.insert(ordered_range, int_even_mset.begin(), int_even_mset.end());
      int_mset4.insert(int_even_mset.begin(), int_even_mset.end());
      if(!CheckEqualContainers(&int_mset4, &fmset))
         return false;
   }
   //Ordered insertion multimap
   {
      std::multimap<int, int> int_mmap;
      for(std::size_t i = 0; i != NumElements; ++i){
         int_mmap.insert(std::multimap<int, int>::value_type(static_cast<int>(i), static_cast<int>(i)));
      }
      //Construction insertion
      flat_multimap<int, int> fmmap(ordered_range, int_mmap.begin(), int_mmap.end());
      if(!CheckEqualContainers(&int_mmap, &fmmap))
         return false;
      //Insertion when empty
      fmmap.clear();
      fmmap.insert(ordered_range, int_mmap.begin(), int_mmap.end());
      if(!CheckEqualContainers(&int_mmap, &fmmap))
         return false;
      //Re-insertion
      fmmap.insert(ordered_range, int_mmap.begin(), int_mmap.end());
      std::multimap<int, int> int_mmap2(int_mmap);
      int_mmap2.insert(int_mmap.begin(), int_mmap.end());
      if(!CheckEqualContainers(&int_mmap2, &fmmap))
         return false;
      //Re-re-insertion
      fmmap.insert(ordered_range, int_mmap2.begin(), int_mmap2.end());
      std::multimap<int, int> int_mmap4(int_mmap2);
      int_mmap4.insert(int_mmap2.begin(), int_mmap2.end());
      if(!CheckEqualContainers(&int_mmap4, &fmmap))
         return false;
      //Re-re-insertion of even
      std::multimap<int, int> int_even_mmap;
      for(std::size_t i = 0; i < NumElements; i+=2){
         int_mmap.insert(std::multimap<int, int>::value_type(static_cast<int>(i), static_cast<int>(i)));
      }
      fmmap.insert(ordered_range, int_even_mmap.begin(), int_even_mmap.end());
      int_mmap4.insert(int_even_mmap.begin(), int_even_mmap.end());
      if(!CheckEqualContainers(&int_mmap4, &fmmap))
         return false;
   }

   //Ordered insertion set
   {
      std::set<int> int_set;
      for(std::size_t i = 0; i != NumElements; ++i){
         int_set.insert(static_cast<int>(i));
      }
      //Construction insertion
      flat_set<int> fset(ordered_unique_range, int_set.begin(), int_set.end());
      if(!CheckEqualContainers(&int_set, &fset))
         return false;
      //Insertion when empty
      fset.clear();
      fset.insert(ordered_unique_range, int_set.begin(), int_set.end());
      if(!CheckEqualContainers(&int_set, &fset))
         return false;
      //Re-insertion
      fset.insert(ordered_unique_range, int_set.begin(), int_set.end());
      std::set<int> int_set2(int_set);
      int_set2.insert(int_set.begin(), int_set.end());
      if(!CheckEqualContainers(&int_set2, &fset))
         return false;
      //Re-re-insertion
      fset.insert(ordered_unique_range, int_set2.begin(), int_set2.end());
      std::set<int> int_set4(int_set2);
      int_set4.insert(int_set2.begin(), int_set2.end());
      if(!CheckEqualContainers(&int_set4, &fset))
         return false;
      //Re-re-insertion of even
      std::set<int> int_even_set;
      for(std::size_t i = 0; i < NumElements; i+=2){
         int_set.insert(static_cast<int>(i));
      }
      fset.insert(ordered_unique_range, int_even_set.begin(), int_even_set.end());
      int_set4.insert(int_even_set.begin(), int_even_set.end());
      if(!CheckEqualContainers(&int_set4, &fset))
         return false;
   }
   //Ordered insertion map
   {
      std::map<int, int> int_map;
      for(std::size_t i = 0; i != NumElements; ++i){
         int_map.insert(std::map<int, int>::value_type(static_cast<int>(i), static_cast<int>(i)));
      }
      //Construction insertion
      flat_map<int, int> fmap(ordered_unique_range, int_map.begin(), int_map.end());
      if(!CheckEqualContainers(&int_map, &fmap))
         return false;
      //Insertion when empty
      fmap.clear();
      fmap.insert(ordered_unique_range, int_map.begin(), int_map.end());
      if(!CheckEqualContainers(&int_map, &fmap))
         return false;
      //Re-insertion
      fmap.insert(ordered_unique_range, int_map.begin(), int_map.end());
      std::map<int, int> int_map2(int_map);
      int_map2.insert(int_map.begin(), int_map.end());
      if(!CheckEqualContainers(&int_map2, &fmap))
         return false;
      //Re-re-insertion
      fmap.insert(ordered_unique_range, int_map2.begin(), int_map2.end());
      std::map<int, int> int_map4(int_map2);
      int_map4.insert(int_map2.begin(), int_map2.end());
      if(!CheckEqualContainers(&int_map4, &fmap))
         return false;
      //Re-re-insertion of even
      std::map<int, int> int_even_map;
      for(std::size_t i = 0; i < NumElements; i+=2){
         int_map.insert(std::map<int, int>::value_type(static_cast<int>(i), static_cast<int>(i)));
      }
      fmap.insert(ordered_unique_range, int_even_map.begin(), int_even_map.end());
      int_map4.insert(int_even_map.begin(), int_even_map.end());
      if(!CheckEqualContainers(&int_map4, &fmap))
         return false;
   }

   return true;
}

}}}

int main()
{
   using namespace boost::container::test;

   //Allocator argument container
   {
      flat_set<int> set_((std::allocator<int>()));
      flat_multiset<int> multiset_((std::allocator<int>()));
      flat_map<int, int> map_((std::allocator<std::pair<int, int> >()));
      flat_multimap<int, int> multimap_((std::allocator<std::pair<int, int> >()));
   }
   //Now test move semantics
   {
      test_move<flat_set<recursive_flat_set> >();
      test_move<flat_multiset<recursive_flat_multiset> >();
      test_move<flat_map<recursive_flat_map, recursive_flat_map> >();
      test_move<flat_multimap<recursive_flat_multimap, recursive_flat_multimap> >();
   }

   if(!flat_tree_ordered_insertion_test()){
      return 1;
   }

   if (0 != set_test<
                  MyBoostSet
                  ,MyStdSet
                  ,MyBoostMultiSet
                  ,MyStdMultiSet>()){
      std::cout << "Error in set_test<MyBoostSet>" << std::endl;
      return 1;
   }

   if (0 != set_test_copyable<
                  MyBoostSet
                  ,MyStdSet
                  ,MyBoostMultiSet
                  ,MyStdMultiSet>()){
      std::cout << "Error in set_test<MyBoostSet>" << std::endl;
      return 1;
   }

   if (0 != set_test<
                  MyMovableBoostSet
                  ,MyStdSet
                  ,MyMovableBoostMultiSet
                  ,MyStdMultiSet>()){
      std::cout << "Error in set_test<MyMovableBoostSet>" << std::endl;
      return 1;
   }

   if (0 != set_test<
                  MyMoveCopyBoostSet
                  ,MyStdSet
                  ,MyMoveCopyBoostMultiSet
                  ,MyStdMultiSet>()){
      std::cout << "Error in set_test<MyMoveCopyBoostSet>" << std::endl;
      return 1;
   }

   if (0 != set_test_copyable<
                  MyMoveCopyBoostSet
                  ,MyStdSet
                  ,MyMoveCopyBoostMultiSet
                  ,MyStdMultiSet>()){
      std::cout << "Error in set_test<MyBoostSet>" << std::endl;
      return 1;
   }

   if (0 != set_test<
                  MyCopyBoostSet
                  ,MyStdSet
                  ,MyCopyBoostMultiSet
                  ,MyStdMultiSet>()){
      std::cout << "Error in set_test<MyCopyBoostSet>" << std::endl;
      return 1;
   }

   if (0 != set_test_copyable<
                  MyCopyBoostSet
                  ,MyStdSet
                  ,MyCopyBoostMultiSet
                  ,MyStdMultiSet>()){
      std::cout << "Error in set_test<MyBoostSet>" << std::endl;
      return 1;
   }

   if (0 != map_test<
                  MyBoostMap
                  ,MyStdMap
                  ,MyBoostMultiMap
                  ,MyStdMultiMap>()){
      std::cout << "Error in set_test<MyBoostMap>" << std::endl;
      return 1;
   }

   if (0 != map_test_copyable<
                  MyBoostMap
                  ,MyStdMap
                  ,MyBoostMultiMap
                  ,MyStdMultiMap>()){
      std::cout << "Error in set_test<MyBoostMap>" << std::endl;
      return 1;
   }

   if (0 != map_test<
                  MyMovableBoostMap
                  ,MyStdMap
                  ,MyMovableBoostMultiMap
                  ,MyStdMultiMap>()){
      return 1;
   }

   if (0 != map_test<
                  MyMoveCopyBoostMap
                  ,MyStdMap
                  ,MyMoveCopyBoostMultiMap
                  ,MyStdMultiMap>()){
      std::cout << "Error in set_test<MyMoveCopyBoostMap>" << std::endl;
      return 1;
   }

   if (0 != map_test_copyable<
                  MyMoveCopyBoostMap
                  ,MyStdMap
                  ,MyMoveCopyBoostMultiMap
                  ,MyStdMultiMap>()){
      std::cout << "Error in set_test<MyBoostMap>" << std::endl;
      return 1;
   }

   if (0 != map_test<
                  MyCopyBoostMap
                  ,MyStdMap
                  ,MyCopyBoostMultiMap
                  ,MyStdMultiMap>()){
      std::cout << "Error in set_test<MyCopyBoostMap>" << std::endl;
      return 1;
   }

   if (0 != map_test_copyable<
                  MyCopyBoostMap
                  ,MyStdMap
                  ,MyCopyBoostMultiMap
                  ,MyStdMultiMap>()){
      std::cout << "Error in set_test<MyBoostMap>" << std::endl;
      return 1;
   }

   const test::EmplaceOptions SetOptions = (test::EmplaceOptions)(test::EMPLACE_HINT | test::EMPLACE_ASSOC);
   const test::EmplaceOptions MapOptions = (test::EmplaceOptions)(test::EMPLACE_HINT_PAIR | test::EMPLACE_ASSOC_PAIR);

   if(!boost::container::test::test_emplace<flat_map<test::EmplaceInt, test::EmplaceInt>, MapOptions>())
      return 1;
   if(!boost::container::test::test_emplace<flat_multimap<test::EmplaceInt, test::EmplaceInt>, MapOptions>())
      return 1;
   if(!boost::container::test::test_emplace<flat_set<test::EmplaceInt>, SetOptions>())
      return 1;
   if(!boost::container::test::test_emplace<flat_multiset<test::EmplaceInt>, SetOptions>())
      return 1;
   if(!boost::container::test::test_propagate_allocator<flat_tree_propagate_test_wrapper>())
      return 1;

   return 0;
}

#include <boost/container/detail/config_end.hpp>

