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
//[doc_value_traits_code_legacy
#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <vector>

//This node is the legacy type we can't modify and we want to insert in
//intrusive list and slist containers using only two pointers, since
//we know the object will never be at the same time in both lists.
struct legacy_value
{
   legacy_value *prev_;
   legacy_value *next_;
   int id_;
};
//]

//[doc_value_traits_value_traits
//Define our own NodeTraits that will configure singly and doubly linked
//list algorithms. Note that this node traits is compatible with
//circular_slist_algorithms and circular_list_algorithms.

namespace bi = boost::intrusive;

struct legacy_node_traits
{
   typedef legacy_value                            node;
   typedef legacy_value *                          node_ptr;
   typedef const legacy_value *                    const_node_ptr;

   static node *get_next(const node *n)            {  return n->next_;  }
   static void set_next(node *n, node *next)       {  n->next_ = next;  }
   static node *get_previous(const node *n)        {  return n->prev_;  }
   static void set_previous(node *n, node *prev)   {  n->prev_ = prev;  }
};

//This ValueTraits will configure list and slist. In this case,
//legacy_node_traits::node is the same as the
//legacy_value_traits::value_type so to_node_ptr/to_value_ptr
//functions are trivial.
struct legacy_value_traits
{
   typedef legacy_node_traits                                  node_traits;
   typedef node_traits::node_ptr                               node_ptr;
   typedef node_traits::const_node_ptr                         const_node_ptr;
   typedef legacy_value                                        value_type;
   typedef legacy_value *                                      pointer;
   typedef const legacy_value *                                const_pointer;
   static const bi::link_mode_type link_mode = bi::normal_link;
   static node_ptr to_node_ptr (value_type &value)             {  return node_ptr(&value); }
   static const_node_ptr to_node_ptr (const value_type &value) {  return const_node_ptr(&value); }
   static pointer to_value_ptr(node_ptr n)                     {  return pointer(n); }
   static const_pointer to_value_ptr(const_node_ptr n)         {  return const_pointer(n); }
};

//]

//[doc_value_traits_test
//Now define an intrusive list and slist that will store legacy_value objects
typedef bi::value_traits<legacy_value_traits>      ValueTraitsOption;
typedef bi::list<legacy_value, ValueTraitsOption>  LegacyAbiList;
typedef bi::slist<legacy_value, ValueTraitsOption> LegacyAbiSlist;

template<class List>
bool test_list()
{
   typedef std::vector<legacy_value> Vect;

   //Create legacy_value objects, with a different internal number
   Vect legacy_vector;
   for(int i = 0; i < 100; ++i){
      legacy_value value;     value.id_ = i;    legacy_vector.push_back(value);
   }

   //Create the list with the objects
   List mylist(legacy_vector.begin(), legacy_vector.end());

   //Now test both lists
   typename List::const_iterator bit(mylist.begin()), bitend(mylist.end());
   typename Vect::const_iterator it(legacy_vector.begin()), itend(legacy_vector.end());

   //Test the objects inserted in our list
   for(; it != itend; ++it, ++bit)
      if(&*bit != &*it) return false;
   return true;
}

int main()
{
   return test_list<LegacyAbiList>() && test_list<LegacyAbiSlist>() ? 0 : 1;
}
//]
