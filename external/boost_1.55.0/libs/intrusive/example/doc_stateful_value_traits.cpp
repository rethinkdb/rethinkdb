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
//[doc_stateful_value_traits
#include <boost/intrusive/list.hpp>

using namespace boost::intrusive;

//This type is not modifiable so we can't store hooks or custom nodes
typedef int identifier_t;

//This value traits will associate elements from an array of identifiers with
//elements of an array of nodes. The element i of the value array will use the
//node i of the node array:
struct stateful_value_traits
{
   typedef list_node_traits<void*>           node_traits;
   typedef node_traits::node                 node;
   typedef node *                            node_ptr;
   typedef const node *                      const_node_ptr;
   typedef identifier_t                      value_type;
   typedef identifier_t *                    pointer;
   typedef const identifier_t *              const_pointer;
   static const link_mode_type link_mode =   normal_link;

   stateful_value_traits(pointer ids, node_ptr node_array)
      :  ids_(ids),  nodes_(node_array)
   {}

   ///Note: non static functions!
   node_ptr to_node_ptr (value_type &value)
      {  return this->nodes_ + (&value - this->ids_); }
   const_node_ptr to_node_ptr (const value_type &value) const
      {  return this->nodes_ + (&value - this->ids_); }
   pointer to_value_ptr(node_ptr n)
      {  return this->ids_ + (n - this->nodes_); }
   const_pointer to_value_ptr(const_node_ptr n) const
      {  return this->ids_ + (n - this->nodes_); }

   private:
   pointer  ids_;
   node_ptr nodes_;
};

int main()
{
   const int NumElements = 100;

   //This is an array of ids that we want to "store"
   identifier_t                  ids   [NumElements];

   //This is an array of nodes that is necessary to form the linked list
   list_node_traits<void*>::node nodes [NumElements];

   //Initialize id objects, each one with a different number
   for(int i = 0; i != NumElements; ++i)   ids[i] = i;

   //Define a list that will "link" identifiers using external nodes
   typedef list<identifier_t, value_traits<stateful_value_traits> > List;

   //This list will store ids without modifying identifier_t instances
   //Stateful value traits must be explicitly passed in the constructor.
   List  my_list (stateful_value_traits (ids, nodes));

   //Insert ids in reverse order in the list
   for(identifier_t * it(&ids[0]), *itend(&ids[NumElements]); it != itend; ++it)
      my_list.push_front(*it);

   //Now test lists
   List::const_iterator   list_it (my_list.cbegin());
   identifier_t *it_val(&ids[NumElements-1]), *it_rbeg_val(&ids[0]-1);

   //Test the objects inserted in the base hook list
   for(; it_val != it_rbeg_val; --it_val, ++list_it)
      if(&*list_it  != &*it_val)   return 1;

   return 0;
}
//]
