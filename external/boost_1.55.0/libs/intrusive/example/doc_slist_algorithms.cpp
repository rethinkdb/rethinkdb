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
//[doc_slist_algorithms_code
#include <boost/intrusive/circular_slist_algorithms.hpp>
#include <cassert>

struct my_node
{
   my_node *next_;
   //other members...
};

//Define our own slist_node_traits
struct my_slist_node_traits
{
   typedef my_node                                 node;
   typedef my_node *                               node_ptr;
   typedef const my_node *                         const_node_ptr;
   static node_ptr get_next(const_node_ptr n)      {  return n->next_;  }
   static void set_next(node_ptr n, node_ptr next) {  n->next_ = next;  }
};

int main()
{
   typedef boost::intrusive::circular_slist_algorithms<my_slist_node_traits> algo;
   my_node one, two, three;

   //Create an empty singly linked list container:
   //"one" will be the first node of the container
   algo::init_header(&one);
   assert(algo::count(&one) == 1);

   //Now add a new node
   algo::link_after(&one, &two);
   assert(algo::count(&one) == 2);

   //Now add a new node after "one"
   algo::link_after(&one, &three);
   assert(algo::count(&one) == 3);

   //Now unlink the node after one
   algo::unlink_after(&one);
   assert(algo::count(&one) == 2);

   //Now unlink two
   algo::unlink(&two);
   assert(algo::count(&one) == 1);

   return 0;
}
//]
