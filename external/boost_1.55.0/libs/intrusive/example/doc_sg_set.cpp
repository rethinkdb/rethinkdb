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
//[doc_sg_set_code
#include <boost/intrusive/sg_set.hpp>
#include <vector>
#include <algorithm>
#include <cassert>

using namespace boost::intrusive;

class MyClass : public bs_set_base_hook<>
{
   int int_;

   public:
   //This is a member hook
   bs_set_member_hook<> member_hook_;

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

//Define an sg_set using the base hook that will store values in reverse order
//and won't execute floating point operations.
typedef sg_set
   < MyClass, compare<std::greater<MyClass> >, floating_point<false> >   BaseSet;

//Define an multiset using the member hook
typedef member_hook<MyClass, bs_set_member_hook<>, &MyClass::member_hook_> MemberOption;
typedef sg_multiset< MyClass, MemberOption>   MemberMultiset;

int main()
{
   typedef std::vector<MyClass>::iterator VectIt;

   //Create several MyClass objects, each one with a different value
   std::vector<MyClass> values;
   for(int i = 0; i < 100; ++i)  values.push_back(MyClass(i));

   BaseSet baseset;
   MemberMultiset membermultiset;

   //Now insert them in the reverse order in the base hook sg_set
   for(VectIt it(values.begin()), itend(values.end()); it != itend; ++it){
      baseset.insert(*it);
      membermultiset.insert(*it);
   }

   //Change balance factor
   membermultiset.balance_factor(0.9f);

   //Now test sg_sets
   {
      BaseSet::reverse_iterator rbit(baseset.rbegin());
      MemberMultiset::iterator mit(membermultiset.begin());
      VectIt it(values.begin()), itend(values.end());

      //Test the objects inserted in the base hook sg_set
      for(; it != itend; ++it, ++rbit)
         if(&*rbit != &*it)   return 1;

      //Test the objects inserted in the member hook sg_set
      for(it = values.begin(); it != itend; ++it, ++mit)
         if(&*mit != &*it) return 1;
   }
   return 0;
}
//]
