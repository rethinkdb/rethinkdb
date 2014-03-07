/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2008-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
//[doc_any_hook
#include <vector>
#include <boost/intrusive/any_hook.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/list.hpp>

using namespace boost::intrusive;

class MyClass : public any_base_hook<> //Base hook
{
   int int_;

   public:
   any_member_hook<> member_hook_;  //Member hook

   MyClass(int i = 0) : int_(i)
   {}
};

int main()
{
   //Define a base hook option that converts any_base_hook to a slist hook
   typedef any_to_slist_hook < base_hook< any_base_hook<> > >     BaseSlistOption;
   typedef slist<MyClass, BaseSlistOption>                        BaseSList;

   //Define a member hook option that converts any_member_hook to a list hook
   typedef any_to_list_hook< member_hook
         < MyClass, any_member_hook<>, &MyClass::member_hook_> >  MemberListOption;
   typedef list<MyClass, MemberListOption>                        MemberList;

   //Create several MyClass objects, each one with a different value
   std::vector<MyClass> values;
   for(int i = 0; i < 100; ++i){ values.push_back(MyClass(i)); }

   BaseSList base_slist;   MemberList member_list;

   //Now insert them in reverse order in the slist and in order in the list
   for(std::vector<MyClass>::iterator it(values.begin()), itend(values.end()); it != itend; ++it)
      base_slist.push_front(*it), member_list.push_back(*it);

   //Now test lists
   BaseSList::iterator bit(base_slist.begin());
   MemberList::reverse_iterator mrit(member_list.rbegin());
   std::vector<MyClass>::reverse_iterator rit(values.rbegin()), ritend(values.rend());

   //Test the objects inserted in the base hook list
   for(; rit != ritend; ++rit, ++bit, ++mrit)
      if(&*bit != &*rit || &*mrit != &*rit)  return 1;
   return 0;
}
//]
