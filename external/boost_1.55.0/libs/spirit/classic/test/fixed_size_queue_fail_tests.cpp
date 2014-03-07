/*=============================================================================
    Copyright (c) 2001-2003 Daniel Nuffer
    Copyright (c) 2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/classic_fixed_size_queue.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/concept_check.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <iostream>

typedef BOOST_SPIRIT_CLASSIC_NS::fixed_size_queue<int, 5> queue_t;
typedef queue_t::iterator iter_t;
typedef queue_t::const_iterator const_iter_t;
BOOST_CLASS_REQUIRE(const_iter_t, boost, RandomAccessIteratorConcept);

// Right now, the iterator is not a full compliant MutableRandomAccessIterator
//  because operator[] does not return a reference. This seems a problem in
//  boost::iterator_adaptors. Anyway, this feature is not used in multi_pass
//  iterator, and this class is not really meant for public use yet.
BOOST_CLASS_REQUIRE(iter_t, boost, RandomAccessIteratorConcept);

int main(int, char**)
{
    // Iterators are random access.
    BOOST_MPL_ASSERT(( boost::is_same<
        iter_t::iterator_category,
        std::random_access_iterator_tag > ));
    BOOST_MPL_ASSERT(( boost::is_same<
        const_iter_t::iterator_category,
        std::random_access_iterator_tag > ));

    queue_t q;
    const queue_t& cq = q;

    iter_t b = q.begin();
    const_iter_t c = cq.begin();

//  MSVC7.1 and EDG aren't able to compile this code for the new iterator
//  adaptors

//  The problem here is, that the old fixed_size_queue code wasn't a complete
//  and 'clean' iterator implementation, some of the required iterator concepts
//  were missing. It was never meant to be exposed outside the multi_pass. So I
//  haven't added any features while porting. The #ifdef'ed tests expose the
//  code weaknesses ((un-)fortunately only on conformant compilers, with a quite
//  good STL implementation). The simplest way to solve this issue was to switch
//  of the tests for these compilers then.

//  Check comparisons and interoperations (we are comparing
//  const and non-const iterators)

    (void) c == b;
    (void) c+4 > b;
    (void) c < b+4;

    return boost::report_errors();
}

