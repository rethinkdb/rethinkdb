//
// Copyright Thomas Witt 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/iterator/iterator_archetypes.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_concepts.hpp>
#include <boost/concept_check.hpp>
#include <boost/cstdlib.hpp>

int main()
{
  {
    typedef boost::iterator_archetype<
        int
      , boost::iterator_archetypes::readable_iterator_t
      , boost::random_access_traversal_tag
    > iter;

    boost::function_requires< boost_concepts::ReadableIteratorConcept<iter> >();
    boost::function_requires< boost_concepts::RandomAccessTraversalConcept<iter> >();
  } 
  {
    typedef boost::iterator_archetype<
        int
      , boost::iterator_archetypes::readable_writable_iterator_t
      , boost::random_access_traversal_tag
    > iter;

    boost::function_requires< boost_concepts::ReadableIteratorConcept<iter> >();
    boost::function_requires< boost_concepts::WritableIteratorConcept<iter> >();
    boost::function_requires< boost_concepts::RandomAccessTraversalConcept<iter> >();
  } 
  {
    typedef boost::iterator_archetype<
        const int // I don't like adding const to Value. It is redundant. -JGS
      , boost::iterator_archetypes::readable_lvalue_iterator_t
      , boost::random_access_traversal_tag
    > iter;

    boost::function_requires< boost_concepts::ReadableIteratorConcept<iter> >();
    boost::function_requires< boost_concepts::LvalueIteratorConcept<iter> >();
    boost::function_requires< boost_concepts::RandomAccessTraversalConcept<iter> >();
  } 
  {
    typedef boost::iterator_archetype<
        int
      , boost::iterator_archetypes::writable_lvalue_iterator_t
      , boost::random_access_traversal_tag
    > iter;

    boost::function_requires< boost_concepts::WritableIteratorConcept<iter> >();
    boost::function_requires< boost_concepts::LvalueIteratorConcept<iter> >();
    boost::function_requires< boost_concepts::RandomAccessTraversalConcept<iter> >();
  } 

  return boost::exit_success;
}

