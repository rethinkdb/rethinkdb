/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2009-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
//[doc_positional_insertion
#include <boost/intrusive/set.hpp>
#include <vector>
#include <algorithm>
#include <cassert>

using namespace boost::intrusive;

//A simple class with a set hook
class MyClass : public set_base_hook<>
{
   public:
   int int_;

   MyClass(int i) :  int_(i) {}
   friend bool operator< (const MyClass &a, const MyClass &b)
      {  return a.int_ < b.int_;  }
   friend bool operator> (const MyClass &a, const MyClass &b)
      {  return a.int_ > b.int_;  }
};

int main()
{
   //Create some ORDERED elements
   std::vector<MyClass> values;
   for(int i = 0; i < 100; ++i)  values.push_back(MyClass(i));

   {  //Data is naturally ordered in the vector with the same criteria
      //as multiset's comparison predicate, so we can just push back
      //all elements, which is more efficient than normal insertion
      multiset<MyClass> mset;
      for(int i = 0; i < 100; ++i)  mset.push_back(values[i]);

      //Now check orderd invariant
      multiset<MyClass>::const_iterator next(mset.cbegin()), it(next++);
      for(int i = 0; i < 99; ++i, ++it, ++next) assert(*it < *next);
   }
   {  //Now the correct order for the set is the reverse order
      //so let's push front all elements
      multiset<MyClass, compare< std::greater<MyClass> > > mset;
      for(int i = 0; i < 100; ++i)  mset.push_front(values[i]);

      //Now check orderd invariant
      multiset<MyClass, compare< std::greater<MyClass> > >::
         const_iterator next(mset.cbegin()), it(next++);
      for(int i = 0; i < 99; ++i, ++it, ++next) assert(*it > *next);
   }
   {  //Now push the first and the last and insert the rest
      //before the last position using "insert_before"
      multiset<MyClass> mset;
      mset.insert_before(mset.begin(), values[0]);
      multiset<MyClass>::const_iterator pos =
         mset.insert_before(mset.end(), values[99]);
      for(int i = 1; i < 99; ++i)  mset.insert_before(pos, values[i]);

      //Now check orderd invariant
      multiset<MyClass>::const_iterator next(mset.cbegin()), it(next++);
      for(int i = 0; i < 99; ++i, ++it, ++next) assert(*it < *next);
   }

   return 0;
}
//]
