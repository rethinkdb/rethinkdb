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
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <vector>

using namespace boost::intrusive;

struct VirtualBase
{
   virtual ~VirtualBase(){}
};

struct VirtualBase2
{
   virtual ~VirtualBase2(){}
};

struct VirtualBase3
{
};

class NonVirtualBase
   :  public virtual VirtualBase
   ,  public virtual VirtualBase2
{
   public:
   int dummy[10];
};

class MyClass
   :  public NonVirtualBase
   ,  public virtual VirtualBase3
{
   int int_;
   public:
   list_member_hook<> list_hook_;
   MyClass(int i = 0)
      :  int_(i)
   {}
};

//Define a list that will store MyClass using the public base hook
typedef member_hook< MyClass, list_member_hook<>, &MyClass::list_hook_ > MemberHook;
typedef list<MyClass, MemberHook> List;

int main()
{
   #ifndef _MSC_VER
   typedef std::vector<MyClass>::iterator VectIt;
   typedef std::vector<MyClass>::reverse_iterator VectRit;

   //Create several MyClass objects, each one with a different value
   std::vector<MyClass> values;
   for(int i = 0; i < 100; ++i)  values.push_back(MyClass(i));

   List  my_list;

   //Now insert them in the reverse order
   //in the base hook intrusive list
   for(VectIt it(values.begin()), itend(values.end()); it != itend; ++it)
      my_list.push_front(*it);

   //Now test lists
   {
      List::const_iterator  list_it(my_list.cbegin());
      VectRit vect_it(values.rbegin()), vect_itend(values.rend());

      //Test the objects inserted in the base hook list
      for(; vect_it != vect_itend; ++vect_it, ++list_it)
         if(&*list_it  != &*vect_it)
            return 1;
   }
   #endif
   return 0;
}
