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
//[doc_how_to_use_code
#include <boost/intrusive/list.hpp>
#include <vector>

using namespace boost::intrusive;

class MyClass : public list_base_hook<>
{
   int int_;

   public:
   list_member_hook<> member_hook_;

   MyClass(int i) :  int_(i)  {}
};

//Define a list that will store MyClass using the base hook
typedef list<MyClass> BaseList;

//Define a list that will store MyClass using the member hook
typedef member_hook
   < MyClass, list_member_hook<>, &MyClass::member_hook_> MemberOption;
typedef list<MyClass, MemberOption> MemberList;

int main()
{
   typedef std::vector<MyClass>::iterator VectIt;
   typedef std::vector<MyClass>::reverse_iterator VectRit;

   //Create several MyClass objects, each one with a different value
   std::vector<MyClass> values;
   for(int i = 0; i < 100; ++i)  values.push_back(MyClass(i));

   BaseList baselist;
   MemberList memberlist;

   //Now insert them in the reverse order in the base hook list
   for(VectIt it(values.begin()), itend(values.end())
      ; it != itend  ; ++it){
      baselist.push_front(*it);
   }

   //Now insert them in the same order as in vector in the member hook list
   for(VectIt it(values.begin()), itend(values.end()); it != itend; ++it)
      memberlist.push_back(*it);

   //Now test lists
   {
      BaseList::reverse_iterator rbit(baselist.rbegin());
      MemberList::iterator mit(memberlist.begin());
      VectIt  it(values.begin()), itend(values.end());

      //Test the objects inserted in the base hook list
      for(; it != itend; ++it, ++rbit)
         if(&*rbit != &*it)   return 1;

      //Test the objects inserted in the member hook list
      for(it = values.begin(); it != itend; ++it, ++mit)
         if(&*mit != &*it)    return 1;
   }

   return 0;
}
//]
