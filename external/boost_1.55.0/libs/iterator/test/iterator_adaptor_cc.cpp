// Copyright (C) 2004 Jeremy Siek <jsiek@cs.indiana.edu>
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/iterator/reverse_iterator.hpp>
#include <boost/iterator/iterator_concepts.hpp>
#include <boost/concept_check.hpp>
#include <boost/cstdlib.hpp>
#include <list>

int main()
{
  {
    typedef boost::reverse_iterator<int*>       rev_iter;
    typedef boost::reverse_iterator<int const*> c_rev_iter;

    boost::function_requires< boost_concepts::WritableIteratorConcept<rev_iter> >();
    boost::function_requires< boost_concepts::LvalueIteratorConcept<rev_iter> >();
    boost::function_requires< boost_concepts::RandomAccessTraversalConcept<rev_iter> >();
    boost::function_requires< boost::RandomAccessIteratorConcept<rev_iter> >();
    boost::function_requires< boost_concepts::InteroperableIteratorConcept<rev_iter, c_rev_iter> >();
  }

    // Many compilers' builtin container iterators don't interoperate well, though
    // STLport fixes that problem.
#if defined(__SGI_STL_PORT)                                                             \
    || !BOOST_WORKAROUND(__GNUC__, <= 2)                                                \
    && !(BOOST_WORKAROUND(__GNUC__, == 3) && BOOST_WORKAROUND(__GNUC_MINOR__, <= 1))    \
    && !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x551))                          \
    && !BOOST_WORKAROUND(__LIBCOMO_VERSION__, BOOST_TESTED_AT(29))                      \
    && !BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, <= 1)
  {
    typedef boost::reverse_iterator<std::list<int>::iterator>       rev_iter;
    typedef boost::reverse_iterator<std::list<int>::const_iterator> c_rev_iter;

    boost::function_requires< boost_concepts::ReadableIteratorConcept<c_rev_iter> >();
    boost::function_requires< boost_concepts::LvalueIteratorConcept<c_rev_iter> >();
    boost::function_requires< boost_concepts::BidirectionalTraversalConcept<c_rev_iter> >();
    boost::function_requires< boost::BidirectionalIteratorConcept<c_rev_iter> >();
    boost::function_requires< boost_concepts::InteroperableIteratorConcept<rev_iter, c_rev_iter> >();
  }
#endif
  
  return boost::exit_success;
}
