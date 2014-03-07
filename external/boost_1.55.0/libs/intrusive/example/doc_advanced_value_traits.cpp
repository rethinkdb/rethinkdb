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
//[doc_advanced_value_traits_code
#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/list.hpp>
#include <vector>

//This is the node that will be used with algorithms.
struct simple_node
{
   simple_node *prev_;
   simple_node *next_;
};
//]

//[doc_advanced_value_traits_value_traits
class base_1{};
class base_2{};

struct value_1 :  public base_1, public simple_node
{  int   id_;  };

struct value_2 :  public base_1, public base_2, public simple_node
{  float id_;  };

//Define the node traits. A single node_traits will be enough.
struct simple_node_traits
{
   typedef simple_node                             node;
   typedef node *                                  node_ptr;
   typedef const node *                            const_node_ptr;
   static node *get_next(const node *n)            {  return n->next_;  }
   static void set_next(node *n, node *next)       {  n->next_ = next;  }
   static node *get_previous(const node *n)        {  return n->prev_;  }
   static void set_previous(node *n, node *prev)   {  n->prev_ = prev;  }
};

//A templatized value traits for value_1 and value_2
template<class ValueType>
struct simple_value_traits
{
   typedef simple_node_traits                                  node_traits;
   typedef node_traits::node_ptr                               node_ptr;
   typedef node_traits::const_node_ptr                         const_node_ptr;
   typedef ValueType                                           value_type;
   typedef ValueType *                                         pointer;
   typedef const ValueType *                                   const_pointer;
   static const boost::intrusive::link_mode_type link_mode = boost::intrusive::normal_link;
   static node_ptr to_node_ptr (value_type &value)             {  return node_ptr(&value); }
   static const_node_ptr to_node_ptr (const value_type &value) {  return const_node_ptr(&value); }
   static pointer to_value_ptr(node_ptr n)                     {  return static_cast<value_type*>(n); }
   static const_pointer to_value_ptr(const_node_ptr n)         {  return static_cast<const value_type*>(n); }
};
//]

//[doc_advanced_value_traits_containers
//Now define two intrusive lists. Both lists will use the same algorithms:
// circular_list_algorithms<simple_node_traits>

using namespace boost::intrusive;
typedef list <value_1, value_traits<simple_value_traits<value_1> > > Value1List;
typedef list <value_2, value_traits<simple_value_traits<value_2> > > Value2List;
//]

//[doc_advanced_value_traits_test
int main()
{
   typedef std::vector<value_1> Vect1;
   typedef std::vector<value_2> Vect2;

   //Create values, with a different internal number
   Vect1 values1;
   Vect2 values2;
   for(int i = 0; i < 100; ++i){
      value_1 v1;    v1.id_ = i;          values1.push_back(v1);
      value_2 v2;    v2.id_ = (float)i;   values2.push_back(v2);
   }

   //Create the lists with the objects
   Value1List list1(values1.begin(), values1.end());
   Value2List list2(values2.begin(), values2.end());

   //Now test both lists
   Value1List::const_iterator bit1(list1.begin()), bitend1(list1.end());
   Value2List::const_iterator bit2(list2.begin()), bitend2(list2.end());

   Vect1::const_iterator it1(values1.begin()), itend1(values1.end());
   Vect2::const_iterator it2(values2.begin()), itend2(values2.end());

   //Test the objects inserted in our lists
   for(; it1 != itend1; ++it1, ++bit1,  ++it2, ++bit2){
      if(&*bit1 != &*it1 || &*bit2 != &*it2) return false;
   }
   return 0;
}
//]
