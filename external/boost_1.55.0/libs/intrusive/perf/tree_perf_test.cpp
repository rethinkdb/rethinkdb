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

//Includes for tests
#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/config.hpp>
#include <iostream>
#include <vector>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/sg_set.hpp>
#include <boost/intrusive/avl_set.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cstdlib>

using namespace boost::posix_time;
using namespace boost::intrusive;

template<bool BigSize>  struct filler        {  int dummy[10];   };
template <>             struct filler<false> {};

template<bool BigSize> //The object for non-intrusive containers
struct test_class :  private filler<BigSize>
{
   std::size_t i_;
   friend bool operator <(const test_class &l, const test_class &r)  {  return l.i_ < r.i_;  }
   friend bool operator >(const test_class &l, const test_class &r)  {  return l.i_ > r.i_;  }
};

template <bool BigSize, class HookType>
struct itest_class   //The object for intrusive containers
   :  public HookType,  public test_class<BigSize>
{
};

#ifdef NDEBUG
const std::size_t NumElem = 1000000;
#else
const std::size_t NumElem = 10000;
#endif
const std::size_t NumRepeat = 4;

enum InsertionType
{
   Monotonic,
   Random
};

template<class Container>
void fill_vector(Container &values, InsertionType insertion_type)
{
   switch(insertion_type){
      case Monotonic:{
         for( typename Container::size_type i = 0, max = values.size()
            ; i != max
            ; ++i){
            values[i].i_ = i;
         }
      }
      break;
      case Random:{
         std::srand(0);
         for( typename Container::size_type i = 0, max = values.size()
            ; i != max
            ; ++i){
            values[i].i_ = i;
         }
         std::random_shuffle(values.begin(), values.end());
      }
      break;
      default:{
         std::abort();
      }
   }
}

template<class Container>
void test_insertion(Container &c, const char *ContainerName, InsertionType insertion_type)
{
   std::cout << "Container " << ContainerName << std::endl;
   //Prepare
   typedef typename Container::size_type  size_type;
   typedef typename Container::value_type value_type;
   ptime tini, tend;
   std::vector<Container::value_type> values(NumElem);
   {
      fill_vector(values, insertion_type);
      //Insert
      tini = microsec_clock::universal_time();
      for( size_type repeat = 0, repeat_max = NumRepeat
         ; repeat != repeat_max
         ; ++repeat){
         c.clear();
         for( size_type i = 0, max = values.size()
            ; i != max
            ; ++i){
               c.insert(values[i]);
         }
         if(c.size() != values.size()){
            std::cout << "    ERROR: size not consistent" << std::endl;
         }
      }
      tend = microsec_clock::universal_time();
      std::cout << "    Insert ns/iter: " << double((tend-tini).total_nanoseconds())/double(NumElem*NumRepeat) << std::endl;
   }
   //Search
   {
      value_type v;
      tini = microsec_clock::universal_time();
      for( size_type repeat = 0, repeat_max = NumRepeat
         ; repeat != repeat_max
         ; ++repeat){
         size_type found = 0;
         for( size_type i = 0, max = values.size()
            ; i != max
            ; ++i){
               v.i_ = i;
               found += static_cast<size_type>(c.end() != c.find(v));
         }
         if(found != NumElem){
            std::cout << "    ERROR: all not found (" << found << ") vs. (" << NumElem << ")" << std::endl;
         }
      }
      tend = microsec_clock::universal_time();
      std::cout << "    Search ns/iter: " << double((tend-tini).total_nanoseconds())/double(NumElem*NumRepeat) << std::endl;
   }
}


void test_insert_search(InsertionType insertion_type)
{
   {
      typedef set_base_hook< link_mode<normal_link> > SetHook;
      typedef set< itest_class<true, SetHook> > Set;
      Set c;
      test_insertion(c, "Set", insertion_type);
   }
   {
      typedef avl_set_base_hook< link_mode<normal_link> > AvlSetHook;
      typedef avl_set< itest_class<true, AvlSetHook> > AvlSet;
      AvlSet c;
      test_insertion(c, "AvlSet", insertion_type);
   }
   {
      typedef bs_set_base_hook< link_mode<normal_link> > BsSetHook;
      typedef sg_set< itest_class<true, BsSetHook> > SgSet;
      SgSet c;
      c.balance_factor(0.55f);
      test_insertion(c, "SgSet(alpha 0.55)", insertion_type);
   }
   {
      typedef bs_set_base_hook< link_mode<normal_link> > BsSetHook;
      typedef sg_set< itest_class<true, BsSetHook> > SgSet;
      SgSet c;
      c.balance_factor(0.60f);
      test_insertion(c, "SgSet(alpha 0.60)", insertion_type);
   }
   {
      typedef bs_set_base_hook< link_mode<normal_link> > BsSetHook;
      typedef sg_set< itest_class<true, BsSetHook> > SgSet;
      SgSet c;
      c.balance_factor(0.65f);
      test_insertion(c, "SgSet(alpha 0.65)", insertion_type);
   }
   {
      typedef bs_set_base_hook< link_mode<normal_link> > BsSetHook;
      typedef sg_set< itest_class<true, BsSetHook> > SgSet;
      SgSet c;
      test_insertion(c, "SgSet(alpha 0.7)", insertion_type);
   }
   {
      typedef bs_set_base_hook< link_mode<normal_link> > BsSetHook;
      typedef sg_set< itest_class<true, BsSetHook> > SgSet;
      SgSet c;
      c.balance_factor(0.75f);
      test_insertion(c, "SgSet(alpha 0.75)", insertion_type);
   }
   {
      typedef bs_set_base_hook< link_mode<normal_link> > BsSetHook;
      typedef sg_set< itest_class<true, BsSetHook> > SgSet;
      SgSet c;
      c.balance_factor(0.80f);
      test_insertion(c, "SgSet(alpha 0.80)", insertion_type);
   }
   {
      typedef bs_set_base_hook< link_mode<normal_link> > BsSetHook;
      typedef sg_set< itest_class<true, BsSetHook>, floating_point<false> > SgSet;
      SgSet c;
      test_insertion(c, "SgSet(no float, alpha 1/sqrt(2)~0,7071)", insertion_type);
   }
}

int main()
{
   std::cout << "MONOTONIC INPUTS\n";
   std::cout << "----------------\n\n";
   test_insert_search(Monotonic);
   std::cout << "----------------\n\n";
   std::cout << "RANDOM INPUTS\n";
   std::cout << "----------------\n\n";
   test_insert_search(Random);
   std::cout << "----------------\n\n";
   return 0;
}

#include <boost/intrusive/detail/config_end.hpp>
