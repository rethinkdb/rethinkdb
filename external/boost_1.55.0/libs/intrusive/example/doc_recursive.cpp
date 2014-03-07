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
//[doc_recursive
#include <boost/intrusive/list.hpp>
#include <cassert>

using namespace boost::intrusive;

typedef list_base_hook<> BaseHook;

//A recursive class
class Recursive : public BaseHook
{
   private:
   Recursive(const Recursive&);
   Recursive & operator=(const Recursive&);

   public:
   Recursive() : BaseHook(), children(){}
   list< Recursive, base_hook<BaseHook> > children;
};

int main()
{
   Recursive f, f2;
   //A recursive list of Recursive
   list< Recursive, base_hook<BaseHook> > l;

   //Insert a node in parent list
   l.insert(l.begin(), f);

   //Insert a node in child list
   l.begin()->children.insert(l.begin()->children.begin(), f2);

   //Objects properly inserted
   assert(l.size() == l.begin()->children.size());
   assert(l.size() == 1);

   //Clear both lists
   l.begin()->children.clear();
   l.clear();
   return 0;
}
//]
