// Boost.Range library
//
//  Copyright Akira Takahashi 2013. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//

#include <boost/range/concepts.hpp>

template <class RandomAccessRng>
void check_random_access_range_concept(const RandomAccessRng& rng)
{
    BOOST_RANGE_CONCEPT_ASSERT(( boost::RandomAccessRangeConcept<RandomAccessRng> ));
}

template <class BidirectionalRng>
void check_bidirectional_range_concept(const BidirectionalRng& rng)
{
    BOOST_RANGE_CONCEPT_ASSERT(( boost::BidirectionalRangeConcept<BidirectionalRng> ));
}
