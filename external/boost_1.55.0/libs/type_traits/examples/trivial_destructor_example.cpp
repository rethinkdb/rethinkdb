
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
 * opt::destroy_array - an example of optimisation based upon omitted destructor calls
 *
 */


#include <iostream>
#include <boost/test/included/prg_exec_monitor.hpp>
#include <boost/timer.hpp>
#include <boost/type_traits.hpp>

using std::cout;
using std::endl;
using std::cin;

namespace opt{

//
// algorithm destroy_array:
// The reverse of std::unitialized_copy, takes a block of
// initialized memory and calls destructors on all objects therein.
//

namespace detail{

template <class T>
void do_destroy_array(T* first, T* last, const boost::false_type&)
{
   while(first != last)
   {
      first->~T();
      ++first;
   }
}

template <class T>
inline void do_destroy_array(T* first, T* last, const boost::true_type&)
{
}

} // namespace detail

template <class T>
inline void destroy_array(T* p1, T* p2)
{
   detail::do_destroy_array(p1, p2, ::boost::has_trivial_destructor<T>());
}

//
// unoptimised versions of destroy_array:
//
template <class T>
void destroy_array1(T* first, T* last)
{
   while(first != last)
   {
      first->~T();
      ++first;
   }
}
template <class T>
void destroy_array2(T* first, T* last)
{
   for(; first != last; ++first) first->~T();
}

} // namespace opt

//
// define some global data:
//
const int array_size = 1000;
int i_array[array_size] = {0,};
const int ci_array[array_size] = {0,};
char c_array[array_size] = {0,};
const char cc_array[array_size] = { 0,};

const int iter_count = 1000000;


int cpp_main(int argc, char* argv[])
{
   boost::timer t;
   double result;
   int i;
   //
   // test destroy_array,
   // compare destruction time of an array of ints
   // with unoptimised form.
   //
   cout << "Measuring times in micro-seconds per 1000 elements processed" << endl << endl;
   cout << "testing destroy_array...\n"
    "[Some compilers may be able to optimise the \"unoptimised\"\n versions as well as type_traits does.]" << endl;
   
   // cache load:
   opt::destroy_array(i_array, i_array + array_size);

   // time optimised version:
   t.restart();
   for(i = 0; i < iter_count; ++i)
   {
      opt::destroy_array(i_array, i_array + array_size);
   }
   result = t.elapsed();
   cout << "destroy_array<int>: " << result << endl;

   // cache load:
   opt::destroy_array1(i_array, i_array + array_size);

   // time unoptimised version #1:
   t.restart();
   for(i = 0; i < iter_count; ++i)
   {
      opt::destroy_array1(i_array, i_array + array_size);
   }
   result = t.elapsed();
   cout << "destroy_array<int>(unoptimised#1): " << result << endl;

   // cache load:
   opt::destroy_array2(i_array, i_array + array_size);

   // time unoptimised version #2:
   t.restart();
   for(i = 0; i < iter_count; ++i)
   {
      opt::destroy_array2(i_array, i_array + array_size);
   }
   result = t.elapsed();
   cout << "destroy_array<int>(unoptimised#2): " << result << endl << endl;

   return 0;
}











