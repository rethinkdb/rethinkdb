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
//[doc_auto_unlink_code
#include <boost/intrusive/list.hpp>
#include <cassert>

using namespace boost::intrusive;

typedef list_base_hook<link_mode<auto_unlink> > auto_unlink_hook;

class MyClass : public auto_unlink_hook
               //This hook removes the node in the destructor
{
   int int_;

   public:
   MyClass(int i = 0)   :  int_(i)  {}
   void unlink()     {  auto_unlink_hook::unlink();  }
   bool is_linked()  {  return auto_unlink_hook::is_linked();  }
};

//Define a list that will store values using the base hook
//The list can't have constant-time size!
typedef list< MyClass, constant_time_size<false> > List;

int main()
{
   //Create the list
   List l;
   {
      //Create myclass and check it's linked
      MyClass myclass;
      assert(myclass.is_linked() == false);

      //Insert the object
      l.push_back(myclass);

      //Check that we have inserted the object
      assert(l.empty() == false);
      assert(&l.front() == &myclass);
      assert(myclass.is_linked() == true);

      //Now myclass' destructor will unlink it
      //automatically
   }

   //Check auto-unlink has been executed
   assert(l.empty() == true);

   {
      //Now test the unlink() function

      //Create myclass and check it's linked
      MyClass myclass;
      assert(myclass.is_linked() == false);

      //Insert the object
      l.push_back(myclass);

      //Check that we have inserted the object
      assert(l.empty() == false);
      assert(&l.front() == &myclass);
      assert(myclass.is_linked() == true);

      //Now unlink the node
      myclass.unlink();

      //Check auto-unlink has been executed
      assert(l.empty() == true);
   }
   return 0;
}
//]
