// Copyright (C) 2004 Jeremy Siek <jsiek@cs.indiana.edu>
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <boost/iterator/permutation_iterator.hpp>
#include <boost/cstdlib.hpp>
#include <assert.h>


int main() {
  using namespace boost;
  int i = 0;

  typedef std::vector< int > element_range_type;
  typedef std::deque< int > index_type;

  static const int element_range_size = 10;
  static const int index_size = 4;

  element_range_type elements( element_range_size );
  for(element_range_type::iterator el_it = elements.begin() ; el_it != elements.end() ; ++el_it)
    *el_it = std::distance(elements.begin(), el_it);

  index_type indices( index_size );
  for(index_type::iterator i_it = indices.begin() ; i_it != indices.end() ; ++i_it ) 
    *i_it = element_range_size - index_size + std::distance(indices.begin(), i_it);
  std::reverse( indices.begin(), indices.end() );

  typedef permutation_iterator< element_range_type::iterator, index_type::iterator > permutation_type;
  permutation_type begin = make_permutation_iterator( elements.begin(), indices.begin() );
  permutation_type it = begin;
  permutation_type end = make_permutation_iterator( elements.begin(), indices.end() );

  std::cout << "The original range is : ";
  std::copy( elements.begin(), elements.end(), std::ostream_iterator< int >( std::cout, " " ) );
  std::cout << "\n";

  std::cout << "The reindexing scheme is : ";
  std::copy( indices.begin(), indices.end(), std::ostream_iterator< int >( std::cout, " " ) );
  std::cout << "\n";

  std::cout << "The permutated range is : ";
  std::copy( begin, end, std::ostream_iterator< int >( std::cout, " " ) );
  std::cout << "\n";

  std::cout << "Elements at even indices in the permutation : ";
  it = begin;
  for(i = 0; i < index_size / 2 ; ++i, it+=2 ) std::cout << *it << " ";
  std::cout << "\n";

  std::cout << "Permutation backwards : ";
  it = begin + (index_size);
  assert( it != begin );
  for( ; it-- != begin ; ) std::cout << *it << " ";
  std::cout << "\n";

  std::cout << "Iterate backward with stride 2 : ";
  it = begin + (index_size - 1);
  for(i = 0 ; i < index_size / 2 ; ++i, it-=2 ) std::cout << *it << " ";
  std::cout << "\n";

  return boost::exit_success;
}
