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
//[doc_slist_code
#include <boost/intrusive/slist.hpp>
#include <vector>

using namespace boost::intrusive;

                  //This is a base hook
class MyClass : public slist_base_hook<>
{
   int int_;

   public:
   //This is a member hook
   slist_member_hook<> member_hook_;

   MyClass(int i)
      :  int_(i)
   {}
};

//Define an slist that will store MyClass using the public base hook
typedef slist<MyClass> BaseList;

//Define an slist that will store MyClass using the public member hook
typedef member_hook<MyClass, slist_member_hook<>, &MyClass::member_hook_> MemberOption;
typedef slist<MyClass, MemberOption> MemberList;

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
   for(VectIt it(values.begin()), itend(values.end()); it != itend; ++it)
      baselist.push_front(*it);

   //Now insert them in the same order as in vector in the member hook list
   for(BaseList::iterator it(baselist.begin()), itend(baselist.end())
      ; it != itend; ++it){
      memberlist.push_front(*it);
   }

   //Now test lists
   {
      BaseList::iterator bit(baselist.begin());
      MemberList::iterator mit(memberlist.begin());
      VectRit rit(values.rbegin()), ritend(values.rend());
      VectIt  it(values.begin()), itend(values.end());

      //Test the objects inserted in the base hook list
      for(; rit != ritend; ++rit, ++bit)
         if(&*bit != &*rit)   return 1;

      //Test the objects inserted in the member hook list
      for(; it != itend; ++it, ++mit)
         if(&*mit != &*it)    return 1;
   }

   return 0;
}
//]
