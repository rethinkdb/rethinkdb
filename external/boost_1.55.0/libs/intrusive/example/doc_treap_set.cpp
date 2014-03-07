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
//[doc_treap_set_code
#include <boost/intrusive/treap_set.hpp>
#include <vector>
#include <algorithm>
#include <cassert>

using namespace boost::intrusive;

class MyClass : public bs_set_base_hook<> //This is a base hook
{
   int int_;
   unsigned int prio_;

   public:
   //This is a member hook
   bs_set_member_hook<> member_hook_;

   MyClass(int i, unsigned int prio)  :  int_(i), prio_(prio)
      {}

   unsigned int get_priority() const
   {  return this->prio_;   }

   //Less and greater operators
   friend bool operator< (const MyClass &a, const MyClass &b)
      {  return a.int_ < b.int_;  }
   friend bool operator> (const MyClass &a, const MyClass &b)
      {  return a.int_ > b.int_;  }
   //Default priority compare
   friend bool priority_order (const MyClass &a, const MyClass &b)
      {  return a.prio_ < b.prio_;  }  //Lower value means higher priority
   //Inverse priority compare
   friend bool priority_inverse_order (const MyClass &a, const MyClass &b)
      {  return a.prio_ > b.prio_;  }  //Higher value means higher priority
};

struct inverse_priority
{
   bool operator()(const MyClass &a, const MyClass &b) const
   {  return priority_inverse_order(a, b); }
};


//Define an treap_set using the base hook that will store values in reverse order
typedef treap_set< MyClass, compare<std::greater<MyClass> > >     BaseSet;

//Define an multiset using the member hook that will store
typedef member_hook<MyClass, bs_set_member_hook<>, &MyClass::member_hook_> MemberOption;
typedef treap_multiset
   < MyClass, MemberOption, priority<inverse_priority> > MemberMultiset;

int main()
{
   typedef std::vector<MyClass>::iterator VectIt;

   //Create several MyClass objects, each one with a different value
   std::vector<MyClass> values;
   for(int i = 0; i < 100; ++i)  values.push_back(MyClass(i, (i % 10)));

   BaseSet baseset;
   MemberMultiset membermultiset;

   //Now insert them in the sets
   for(VectIt it(values.begin()), itend(values.end()); it != itend; ++it){
      baseset.insert(*it);
      membermultiset.insert(*it);
   }

   //Now test treap_sets
   {
      BaseSet::reverse_iterator rbit(baseset.rbegin());
      MemberMultiset::iterator mit(membermultiset.begin());
      VectIt it(values.begin()), itend(values.end());

      //Test the objects inserted in the base hook treap_set
      for(; it != itend; ++it, ++rbit)
         if(&*rbit != &*it)   return 1;

      //Test the objects inserted in the member hook treap_set
      for(it = values.begin(); it != itend; ++it, ++mit)
         if(&*mit != &*it) return 1;

      //Test priority order
      for(int i = 0; i < 100; ++i){
         if(baseset.top()->get_priority() != static_cast<unsigned int>(i/10))
            return 1;
         if(membermultiset.top()->get_priority() != 9u - static_cast<unsigned int>(i/10))
            return 1;
         baseset.erase(baseset.top());
         membermultiset.erase(membermultiset.top());
      }
   }
   return 0;
}
//]
