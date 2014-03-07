/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2006-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
//[doc_avl_set_code
#include <boost/intrusive/avl_set.hpp>
#include <vector>
#include <algorithm>
#include <cassert>

using namespace boost::intrusive;

                  //This is a base hook optimized for size
class MyClass : public avl_set_base_hook<optimize_size<true> >
{
   int int_;

   public:
   //This is a member hook
   avl_set_member_hook<> member_hook_;

   MyClass(int i)
      :  int_(i)
      {}
   friend bool operator< (const MyClass &a, const MyClass &b)
      {  return a.int_ < b.int_;  }
   friend bool operator> (const MyClass &a, const MyClass &b)
      {  return a.int_ > b.int_;  }
   friend bool operator== (const MyClass &a, const MyClass &b)
      {  return a.int_ == b.int_;  }
};

//Define an avl_set using the base hook that will store values in reverse order
typedef avl_set< MyClass, compare<std::greater<MyClass> > >     BaseSet;

//Define an multiset using the member hook
typedef member_hook<MyClass, avl_set_member_hook<>, &MyClass::member_hook_> MemberOption;
typedef avl_multiset< MyClass, MemberOption>   MemberMultiset;

int main()
{
   typedef std::vector<MyClass>::iterator VectIt;

   //Create several MyClass objects, each one with a different value
   std::vector<MyClass> values;
   for(int i = 0; i < 100; ++i)  values.push_back(MyClass(i));

   BaseSet baseset;
   MemberMultiset membermultiset;

   //Check that size optimization is activated in the base hook
   assert(sizeof(avl_set_base_hook<optimize_size<true> >) == 3*sizeof(void*));
   //Check that size optimization is deactivated in the member hook
   assert(sizeof(avl_set_member_hook<>) > 3*sizeof(void*));

   //Now insert them in the sets
   for(VectIt it(values.begin()), itend(values.end()); it != itend; ++it){
      baseset.insert(*it);
      membermultiset.insert(*it);
   }

   //Now test avl_sets
   {
      BaseSet::reverse_iterator rbit(baseset.rbegin());
      MemberMultiset::iterator mit(membermultiset.begin());
      VectIt it(values.begin()), itend(values.end());

      //Test the objects inserted in the base hook avl_set
      for(; it != itend; ++it, ++rbit)
         if(&*rbit != &*it)   return 1;

      //Test the objects inserted in the member hook avl_set
      for(it = values.begin(); it != itend; ++it, ++mit)
         if(&*mit != &*it) return 1;
   }
   return 0;
}
//]
