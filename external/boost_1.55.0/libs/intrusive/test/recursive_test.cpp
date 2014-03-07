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
#include <boost/intrusive/avl_set.hpp>
#include <boost/intrusive/sg_set.hpp>
#include <boost/intrusive/splay_set.hpp>
#include <boost/intrusive/treap_set.hpp>
#include <cassert>

using namespace boost::intrusive;

typedef list_base_hook<>            ListBaseHook;
typedef slist_base_hook<>           SListBaseHook;
typedef set_base_hook<>             SetBaseHook;
typedef unordered_set_base_hook<>   USetBaseHook;

class Foo;
typedef unordered_set<Foo, base_hook<USetBaseHook> > USet;

class Foo : public ListBaseHook, public SListBaseHook, public SetBaseHook, public USetBaseHook
{
   USet::bucket_type buckets[1];
   Foo(const Foo &);
   Foo & operator=(const Foo &);

   public:
   Foo() : uset_children(USet::bucket_traits(buckets, 1))
   {}
   list <Foo, base_hook<ListBaseHook> >  list_children;
   slist<Foo, base_hook<SListBaseHook> > slist_children;
   set  <Foo, base_hook<SetBaseHook> >   set_children;
   USet  uset_children;
};

void instantiate()
{
   list< Foo, base_hook<ListBaseHook> >   list_;   list_.clear();
   slist< Foo, base_hook<SListBaseHook> > slist_;  slist_.clear();
   set< Foo, base_hook<SetBaseHook> > set_;  set_.clear();

   USet::bucket_type buckets[1];
   USet unordered_set_(USet::bucket_traits(buckets, 1));  unordered_set_.clear();
}
int main()
{
   instantiate();

   //A small test with list
   {
      Foo f, f2;
      list< Foo, base_hook<ListBaseHook> > l;
      l.insert(l.begin(), f);
      l.begin()->list_children.insert(l.begin()->list_children.begin(), f2);
      assert(l.size() == l.begin()->list_children.size());
      l.begin()->list_children.clear();
      l.clear();
   }
   return 0;
}
