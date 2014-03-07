
/*
 *
 * (C) Copyright John Maddock 1999-2005. 
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * This file provides some example of type_traits usage -
 * by "optimising" various algorithms:
 *
 * opt::iter_swap - uses type_traits to determine whether the iterator is a proxy
 *                  in which case it uses a "safe" approach, otherwise calls swap
 *                  on the assumption that swap may be specialised for the pointed-to type.
 *
 */

#include <iostream>
#include <typeinfo>
#include <algorithm>
#include <iterator>
#include <vector>
#include <memory>

#include <boost/test/included/prg_exec_monitor.hpp>
#include <boost/type_traits.hpp>

using std::cout;
using std::endl;
using std::cin;

namespace opt{

//
// iter_swap:
// tests whether iterator is a proxy iterator or not, and
// uses optimal form accordingly:
//
namespace detail{

template <typename I>
static void do_swap(I one, I two, const boost::false_type&)
{
   typedef typename std::iterator_traits<I>::value_type v_t;
   v_t v = *one;
   *one = *two;
   *two = v;
}
template <typename I>
static void do_swap(I one, I two, const boost::true_type&)
{
   using std::swap;
   swap(*one, *two);
}

}

template <typename I1, typename I2>
inline void iter_swap(I1 one, I2 two)
{
   //
   // See is both arguments are non-proxying iterators, 
   // and if both iterator the same type:
   //
   typedef typename std::iterator_traits<I1>::reference r1_t;
   typedef typename std::iterator_traits<I2>::reference r2_t;

   typedef boost::integral_constant<bool,
      ::boost::is_reference<r1_t>::value
      && ::boost::is_reference<r2_t>::value
      && ::boost::is_same<r1_t, r2_t>::value> truth_type;

   detail::do_swap(one, two, truth_type());
}


};   // namespace opt

int cpp_main(int argc, char* argv[])
{
   //
   // testing iter_swap
   // really just a check that it does in fact compile...
   std::vector<int> v1;
   v1.push_back(0);
   v1.push_back(1);
   std::vector<bool> v2;
   v2.push_back(0);
   v2.push_back(1);
   opt::iter_swap(v1.begin(), v1.begin()+1);
   opt::iter_swap(v2.begin(), v2.begin()+1);

   return 0;
}



