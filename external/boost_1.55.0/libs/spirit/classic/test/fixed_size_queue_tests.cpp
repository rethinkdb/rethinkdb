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
    queue_t q;
    const queue_t& cq = q;

    q.push_back(1);
    q.push_back(2);
    q.push_back(3);
    q.push_back(4);
    BOOST_TEST(q.front() == 1);
    q.pop_front();
    BOOST_TEST(q.front() == 2);
    q.pop_front();
    BOOST_TEST(q.front() == 3);
    q.pop_front();
    BOOST_TEST(q.front() == 4);
    q.pop_front();
    q.push_back(5);
    q.push_back(6);
    q.push_back(7);
    q.push_back(8);
    BOOST_TEST(q.front() == 5);
    q.pop_front();
    BOOST_TEST(q.front() == 6);
    q.pop_front();
    BOOST_TEST(q.front() == 7);
    q.pop_front();
    BOOST_TEST(q.front() == 8);
    q.pop_front();

    q.push_front(5);
    q.push_front(4);
    q.push_front(3);
    q.push_front(2);
    q.push_front(1);

    // NOTE: Iterator tests are not exhaustive and they are not meant to be so.

    // Check iterator
    iter_t b = q.begin();
    BOOST_TEST(*b++ == 1);
    BOOST_TEST(*b++ == 2);
    BOOST_TEST(*b++ == 3);
    BOOST_TEST(*b++ == 4);
    BOOST_TEST(*b++ == 5);
    BOOST_TEST(b == q.end());
    BOOST_TEST(*--b == 5);
    BOOST_TEST(*--b == 4);
    BOOST_TEST(*--b == 3);
    BOOST_TEST(*--b == 2);
    BOOST_TEST(*--b == 1);
    BOOST_TEST(b == q.begin());

    // Check const_iterator
    const_iter_t c = cq.begin();
    BOOST_TEST(*c++ == 1);
    BOOST_TEST(*c++ == 2);
    BOOST_TEST(*c++ == 3);
    BOOST_TEST(*c++ == 4);
    BOOST_TEST(*c++ == 5);
    BOOST_TEST(c == cq.end());
    BOOST_TEST(*--c == 5);
    BOOST_TEST(*--c == 4);
    BOOST_TEST(*--c == 3);
    BOOST_TEST(*--c == 2);
    BOOST_TEST(*--c == 1);
    BOOST_TEST(c == cq.begin());

#if 0

//  Conforming compilers aren't able to compile this code for the new iterator
//  adaptors.

//  The problem here is, that the old fixed_size_queue code wasn't a complete
//  and 'clean' iterator implementation, some of the required iterator concepts
//  were missing. It was never meant to be exposed outside the multi_pass. So I
//  haven't added any features while porting. The #ifdef'ed tests expose the
//  code weaknesses ((un-)fortunately only on conformant compilers, with a quite
//  good STL implementation). The simplest way to solve this issue was to switch
//  of the tests for these compilers then.

//  $$$ This is isolated in fixed_size_queue_fail_tests.cpp [JDG 11-5-2003] $$$

    // Iterators are random access.
    BOOST_MPL_ASSERT(( boost::is_same<
        iter_t::iterator_category,
        std::random_access_iterator_tag > ));
    BOOST_MPL_ASSERT(( boost::is_same<
        const_iter_t::iterator_category,
        std::random_access_iterator_tag > ));

    //  Check comparisons and interoperations (we are comparing
    //  const and non-const iterators)
    BOOST_TEST(c == b);
    BOOST_TEST(c+4 > b);
    BOOST_TEST(c < b+4);

#endif

    // Check that you can actually modify the queue with an iterator
    *b = 123;
    BOOST_TEST(*c == 123);

    // Check random access
    BOOST_TEST(*((c+4)-4) == 123);
    BOOST_TEST(*((c-4)+4) == 123);

    return boost::report_errors();
}

