//  (C) Copyright John Maddock 2000.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

#include <iterator>
#include <list>
#include <deque>
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>

template <class RandomAccessIterator >
RandomAccessIterator foo(RandomAccessIterator from, RandomAccessIterator)
{
   // this template can only be used with
   // random access iterators...
   typedef typename std::iterator_traits< RandomAccessIterator >::iterator_category cat;
   BOOST_STATIC_ASSERT((boost::is_convertible<cat*, std::random_access_iterator_tag*>::value));
   //
   // detail goes here...
   return from;
}

// ensure that delayed instantiation compilers like Comeau see the failure early
// enough for "compile-fail" testing with the Boost.Build testing framework. (Greg Comeau)
template
std::list<int>::iterator
   foo(std::list<int>::iterator, std::list<int>::iterator);

int main()
{
   std::deque<int> d;
   std::list<int> l;
   foo(d.begin(), d.end()); // OK
   foo(l.begin(), l.end()); // error
   return 0;
}


 


