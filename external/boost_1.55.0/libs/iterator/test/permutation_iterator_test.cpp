// (C) Copyright Toon Knapen    2001.
// (C) Copyright Roland Richter 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp>
#include <boost/test/minimal.hpp>

#include <boost/iterator/permutation_iterator.hpp>
#include <boost/static_assert.hpp>
#include <boost/iterator/iterator_concepts.hpp>
#include <boost/concept/assert.hpp>

#include <vector>
#include <list>

#include <algorithm>


// This test checks for convertibility/interoperability among similar
// permutation iterators.  We're not using container iterators
// underneath, as in permutation_test, because of bugs in GCC-3.3's
// __normal_iterator that make is_convertible choke when testing
// convertibility.
void iterop_test()
{
    typedef boost::permutation_iterator< double*, int const* > permutation_type;
    typedef boost::permutation_iterator< double const*, int const* > permutation_const_type;
  
    BOOST_CONCEPT_ASSERT((
      boost_concepts::InteroperableIteratorConcept<
           permutation_type
         , permutation_const_type
      >));
}

void permutation_test()
{
  // Example taken from documentation of old permutation_iterator.
  typedef std::vector< double > element_range_type;
  typedef std::list< int > index_type;

  const int element_range_size = 10;
  const int index_size = 7;

  BOOST_STATIC_ASSERT(index_size <= element_range_size);
  element_range_type elements( element_range_size );
  for( element_range_type::iterator el_it = elements.begin(); el_it != elements.end(); ++el_it )
    { *el_it = std::distance(elements.begin(), el_it); }

  index_type indices( index_size );
  for( index_type::iterator i_it = indices.begin(); i_it != indices.end(); ++i_it )
    { *i_it = element_range_size - index_size + std::distance(indices.begin(), i_it); }
  std::reverse( indices.begin(), indices.end() );

  typedef boost::permutation_iterator< element_range_type::iterator, index_type::iterator > permutation_type;
  permutation_type begin = boost::make_permutation_iterator( elements.begin(), indices.begin() );
  permutation_type it = begin;
  permutation_type end = boost::make_permutation_iterator( elements.begin(), indices.end() );
  
  BOOST_CHECK( it == begin );
  BOOST_CHECK( it != end );

  BOOST_CHECK( std::distance( begin, end ) == index_size );

  for( index_type::iterator i_it1 = indices.begin(); it != end; ++i_it1, ++it )
  {
    BOOST_CHECK( *it == elements[ *i_it1 ] );
  }

  it = begin;
  for( int i1 = 0; i1 < index_size - 1 ; ++++i1, ++++it ) 
  {
    index_type::iterator i_it2 = indices.begin();
    std::advance( i_it2, i1 );
    BOOST_CHECK( *it == elements[ *i_it2 ] );
  }

  it = begin;
  std::advance(it, index_size);
  for( index_type::iterator i_it3 = indices.end(); it != begin; ) 
  {
    BOOST_CHECK( *--it == elements[ *--i_it3 ] );
  }
  
  it = begin;
  std::advance(it, index_size);
  for( int i2 = 0; i2 < index_size - 1; i2+=2, --it ) 
  {
    index_type::iterator i_it4 = --indices.end();
    std::advance( i_it4, -i2 );
    BOOST_CHECK( *--it == elements[ *i_it4 ] );
  }

}


int test_main(int, char *[])
{
  permutation_test();
  return 0;
}
