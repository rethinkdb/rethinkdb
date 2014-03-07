// Copyright (C) 2004 Jeremy Siek <jsiek@cs.indiana.edu>
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/iterator/iterator_concepts.hpp>
#include <boost/iterator/iterator_archetypes.hpp>
#include <boost/cstdlib.hpp>

int main()
{
  typedef boost::iterator_archetype<
        int
      , boost::iterator_archetypes::readable_iterator_t
      , boost::single_pass_traversal_tag
    > Iter;
  boost::function_requires<
    boost_concepts::LvalueIteratorConcept<Iter> >();
  return boost::exit_success;
}
