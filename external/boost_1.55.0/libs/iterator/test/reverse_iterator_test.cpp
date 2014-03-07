// Copyright Thomas Witt 2003, Jeremy Siek 2004.

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/iterator/reverse_iterator.hpp>
#include <boost/iterator/new_iterator_tests.hpp>
#include <boost/concept_check.hpp>
#include <boost/concept_archetype.hpp>
#include <boost/iterator/iterator_concepts.hpp>
#include <boost/iterator/iterator_archetypes.hpp>
#include <boost/cstdlib.hpp>
#include <algorithm>
#include <deque>
#include <iostream>

using boost::dummyT;

// Test reverse iterator
int main()
{
  dummyT array[] = { dummyT(0), dummyT(1), dummyT(2), 
                     dummyT(3), dummyT(4), dummyT(5) };
  const int N = sizeof(array)/sizeof(dummyT);

  // Concept checks
  // Adapting old-style iterators
  {
    typedef boost::reverse_iterator<boost::bidirectional_iterator_archetype<dummyT> > Iter;
    boost::function_requires< boost::BidirectionalIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::ReadableIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::LvalueIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::BidirectionalTraversalConcept<Iter> >();
  }
  {
    typedef boost::reverse_iterator<boost::mutable_bidirectional_iterator_archetype<dummyT> > Iter;
    boost::function_requires< boost::Mutable_BidirectionalIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::WritableIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::LvalueIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::BidirectionalTraversalConcept<Iter> >();
  }
  // Adapting new-style iterators
  {
    typedef boost::iterator_archetype<
        const dummyT
      , boost::iterator_archetypes::readable_iterator_t
      , boost::bidirectional_traversal_tag
    > iter;
    typedef boost::reverse_iterator<iter> Iter;
    boost::function_requires< boost::InputIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::ReadableIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::BidirectionalTraversalConcept<Iter> >();
  }
#if 0
  // It does not seem feasible to make this work. Need to change docs to
  // require at lease Readable for the base iterator. -Jeremy
  {
    typedef boost::iterator_archetype<
        dummyT
      , boost::iterator_archetypes::writable_iterator_t
      , boost::bidirectional_traversal_tag
    > iter;
    typedef boost::reverse_iterator<iter> Iter;
    boost::function_requires< boost_concepts::WritableIteratorConcept<Iter, dummyT> >();
    boost::function_requires< boost_concepts::BidirectionalTraversalConcept<Iter> >();
  }
#endif
#if !BOOST_WORKAROUND(BOOST_MSVC, == 1200)  // Causes Internal Error in linker.
  {
    typedef boost::iterator_archetype<
        dummyT
      , boost::iterator_archetypes::readable_writable_iterator_t
      , boost::bidirectional_traversal_tag
    > iter;
    typedef boost::reverse_iterator<iter> Iter;
    boost::function_requires< boost::InputIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::ReadableIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::WritableIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::BidirectionalTraversalConcept<Iter> >();
  }
  {
    typedef boost::iterator_archetype<
        const dummyT
      , boost::iterator_archetypes::readable_lvalue_iterator_t
      , boost::bidirectional_traversal_tag
    > iter;
    typedef boost::reverse_iterator<iter> Iter;
    boost::function_requires< boost::BidirectionalIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::ReadableIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::LvalueIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::BidirectionalTraversalConcept<Iter> >();
  }
  {
    typedef boost::iterator_archetype<
        dummyT
      , boost::iterator_archetypes::writable_lvalue_iterator_t
      , boost::bidirectional_traversal_tag
    > iter;
    typedef boost::reverse_iterator<iter> Iter;
    boost::function_requires< boost::BidirectionalIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::WritableIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::LvalueIteratorConcept<Iter> >();
    boost::function_requires< boost_concepts::BidirectionalTraversalConcept<Iter> >();
  }
#endif
  
  // Test reverse_iterator
  {
    dummyT reversed[N];
    std::copy(array, array + N, reversed);
    std::reverse(reversed, reversed + N);
    
    typedef boost::reverse_iterator<dummyT*> reverse_iterator;
    
    reverse_iterator i(reversed + N);
    boost::random_access_iterator_test(i, N, array);

    boost::random_access_iterator_test(boost::make_reverse_iterator(reversed + N), N, array);

    typedef boost::reverse_iterator<const dummyT*> const_reverse_iterator;
    
    const_reverse_iterator j(reversed + N);
    boost::random_access_iterator_test(j, N, array);

    const dummyT* const_reversed = reversed;
    
    boost::random_access_iterator_test(boost::make_reverse_iterator(const_reversed + N), N, array);
    
    boost::const_nonconst_iterator_test(i, ++j);
  }

  // Test reverse_iterator again, with traits fully deducible on all platforms
  {
    std::deque<dummyT> reversed_container;
    std::reverse_copy(array, array + N, std::back_inserter(reversed_container));
    const std::deque<dummyT>::iterator reversed = reversed_container.begin();


    typedef boost::reverse_iterator<
        std::deque<dummyT>::iterator> reverse_iterator;
    typedef boost::reverse_iterator<
        std::deque<dummyT>::const_iterator> const_reverse_iterator;

    // MSVC/STLport gives an INTERNAL COMPILER ERROR when any computation
    // (e.g. "reversed + N") is used in the constructor below.
    const std::deque<dummyT>::iterator finish = reversed_container.end();
    reverse_iterator i(finish);
    
    boost::random_access_iterator_test(i, N, array);
    boost::random_access_iterator_test(boost::make_reverse_iterator(reversed + N), N, array);

    const_reverse_iterator j = reverse_iterator(finish);
    boost::random_access_iterator_test(j, N, array);

    const std::deque<dummyT>::const_iterator const_reversed = reversed;
    boost::random_access_iterator_test(boost::make_reverse_iterator(const_reversed + N), N, array);
    
    // Many compilers' builtin deque iterators don't interoperate well, though
    // STLport fixes that problem.
#if defined(__SGI_STL_PORT)                                                             \
    || !BOOST_WORKAROUND(__GNUC__, <= 2)                                                \
    && !(BOOST_WORKAROUND(__GNUC__, == 3) && BOOST_WORKAROUND(__GNUC_MINOR__, <= 1))    \
    && !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x551))                          \
    && !BOOST_WORKAROUND(__LIBCOMO_VERSION__, BOOST_TESTED_AT(29))                      \
    && !BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, <= 1)
    
    boost::const_nonconst_iterator_test(i, ++j);
    
#endif
  }

  return boost::report_errors();
}
