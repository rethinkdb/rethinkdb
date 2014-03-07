
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
 * opt::fill - optimised for trivial copy/small types (cf std::fill)
 *
 */

#include <iostream>
#include <typeinfo>
#include <algorithm>
#include <iterator>
#include <memory>
#include <cstring>

#include <boost/test/included/prg_exec_monitor.hpp>
#include <boost/timer.hpp>
#include <boost/type_traits.hpp>

#if defined(BOOST_NO_STDC_NAMESPACE) || (defined(std) && defined(__SGI_STL_PORT))
namespace std{ using :: memset; }
#endif

using std::cout;
using std::endl;
using std::cin;

namespace opt{
//
// fill
// same as std::fill, but uses memset where appropriate
//
namespace detail{

template <typename I, typename T, bool b>
void do_fill(I first, I last, const T& val, const boost::integral_constant<bool, b>&)
{
   while(first != last)
   {
      *first = val;
      ++first;
   }
}

template <typename T>
void do_fill(T* first, T* last, const T& val, const boost::true_type&)
{
   std::memset(first, val, last-first);
}

}

template <class I, class T>
inline void fill(I first, I last, const T& val)
{
   //
   // We can do an optimised fill if T has a trivial assignment 
   // operator and if it's size is one:
   //
   typedef boost::integral_constant<bool, 
      ::boost::has_trivial_assign<T>::value && (sizeof(T) == 1)> truth_type;
   detail::do_fill(first, last, val, truth_type());
}

}   // namespace opt

namespace non_opt{

template <class I, class T>
inline void fill(I first, I last, const T& val)
{
   opt::detail::do_fill(first, last, val, boost::false_type());
}

}

//
// define some global data:
//
const int array_size = 1000;
int i_array_[array_size] = {0,};
const int ci_array_[array_size] = {0,};
char c_array_[array_size] = {0,};
const char cc_array_[array_size] = { 0,};
//
// since arrays aren't iterators we define a set of pointer
// aliases into the arrays (otherwise the compiler is entitled
// to deduce the type passed to the template functions as
// T (&)[N] rather than T*).
int* i_array = i_array_;
const int* ci_array = ci_array_;
char* c_array = c_array_;
const char* cc_array = cc_array_;

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

   cout << "testing fill(char)...\n"
   "[Some standard library versions may already perform this optimisation.]" << endl;

   // cache load:
   opt::fill(c_array, c_array + array_size, (char)3);

   // time optimised version:
   t.restart();
   for(i = 0; i < iter_count; ++i)
   {
      opt::fill(c_array, c_array + array_size, (char)3);
   }
   result = t.elapsed();
   cout << "opt::fill<char*, char>: " << result << endl;

   // cache load:
   non_opt::fill(c_array, c_array + array_size, (char)3);

   // time optimised version:
   t.restart();
   for(i = 0; i < iter_count; ++i)
   {
      non_opt::fill(c_array, c_array + array_size, (char)3);
   }
   result = t.elapsed();
   cout << "non_opt::fill<char*, char>: " << result << endl;

   // cache load:
   std::fill(c_array, c_array + array_size, (char)3);

   // time standard version:
   t.restart();
   for(i = 0; i < iter_count; ++i)
   {
      std::fill(c_array, c_array + array_size, (char)3);
   }
   result = t.elapsed();
   cout << "std::fill<char*, char>: " << result << endl << endl;

   cout << "testing fill(int)...\n" << endl;

   // cache load:
   opt::fill(i_array, i_array + array_size, 3);

   // timer optimised version:
   t.restart();
   for(i = 0; i < iter_count; ++i)
   {
      opt::fill(i_array, i_array + array_size, 3);
   }
   result = t.elapsed();
   cout << "opt::fill<int*, int>: " << result << endl;

   // cache load:
   non_opt::fill(i_array, i_array + array_size, 3);

   // timer optimised version:
   t.restart();
   for(i = 0; i < iter_count; ++i)
   {
      non_opt::fill(i_array, i_array + array_size, 3);
   }
   result = t.elapsed();
   cout << "non_opt::fill<int*, int>: " << result << endl;

   // cache load:
   std::fill(i_array, i_array + array_size, 3);

   // time standard version:
   t.restart();
   for(i = 0; i < iter_count; ++i)
   {
      std::fill(i_array, i_array + array_size, 3);
   }
   result = t.elapsed();
   cout << "std::fill<int*, int>: " << result << endl << endl;

   return 0;
}






