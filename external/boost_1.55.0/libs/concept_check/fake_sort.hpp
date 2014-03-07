// Copyright David Abrahams 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_LIBS_CONCEPT_CHECK_FAKE_SORT_DWA2006430_HPP
# define BOOST_LIBS_CONCEPT_CHECK_FAKE_SORT_DWA2006430_HPP

# include <boost/detail/iterator.hpp>
# include <boost/concept/requires.hpp>
# include <boost/concept_check.hpp>

namespace fake
{
  using namespace boost;
  
  template<typename RanIter>
  BOOST_CONCEPT_REQUIRES(
      ((Mutable_RandomAccessIterator<RanIter>))
      ((LessThanComparable<typename Mutable_RandomAccessIterator<RanIter>::value_type>))
    
    , (void))
      sort(RanIter,RanIter)
  {
 
  }
}

#endif // BOOST_LIBS_CONCEPT_CHECK_FAKE_SORT_DWA2006430_HPP
