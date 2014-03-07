/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2007-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
//[doc_avltree_algorithms_code
#include <boost/intrusive/avltree_algorithms.hpp>
#include <cassert>

struct my_node
{
   my_node(int i = 0)
      :  int_(i)
   {}
   my_node *parent_, *left_, *right_;
   int balance_;
   //other members
   int      int_;
};

//Define our own avltree_node_traits
struct my_avltree_node_traits
{
   typedef my_node                                    node;
   typedef my_node *                                  node_ptr;
   typedef const my_node *                            const_node_ptr;
   typedef int                                        balance;

   static node_ptr get_parent(const_node_ptr n)       {  return n->parent_;   }
   static void set_parent(node_ptr n, node_ptr parent){  n->parent_ = parent; }
   static node_ptr get_left(const_node_ptr n)         {  return n->left_;     }
   static void set_left(node_ptr n, node_ptr left)    {  n->left_ = left;     }
   static node_ptr get_right(const_node_ptr n)        {  return n->right_;    }
   static void set_right(node_ptr n, node_ptr right)  {  n->right_ = right;   }
   static balance get_balance(const_node_ptr n)       {  return n->balance_;  }
   static void set_balance(node_ptr n, balance b)     {  n->balance_ = b;     }
   static balance negative()                          {  return -1; }
   static balance zero()                              {  return 0;  }
   static balance positive()                          {  return 1;  }
};

struct node_ptr_compare
{
   bool operator()(const my_node *a, const my_node *b)
   {  return a->int_ < b->int_;  }
};

int main()
{
   typedef boost::intrusive::avltree_algorithms<my_avltree_node_traits> algo;
   my_node header, two(2), three(3);

   //Create an empty avltree container:
   //"header" will be the header node of the tree
   algo::init_header(&header);

   //Now insert node "two" in the tree using the sorting functor
   algo::insert_equal_upper_bound(&header, &two, node_ptr_compare());

   //Now insert node "three" in the tree using the sorting functor
   algo::insert_equal_lower_bound(&header, &three, node_ptr_compare());

   //Now take the first node (the left node of the header)
   my_node *n = header.left_;
   assert(n == &two);

   //Now go to the next node
   n = algo::next_node(n);
   assert(n == &three);

   //Erase a node just using a pointer to it
   algo::unlink(&two);

   //Erase a node using also the header (faster)
   algo::erase(&header, &three);
   return 0;
}

//]
