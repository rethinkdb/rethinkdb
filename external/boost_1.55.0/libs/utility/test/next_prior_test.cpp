// Boost test program for next() and prior() utilities.

// Copyright 2003 Daniel Walker.  Use, modification, and distribution
// are subject to the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/utility for documentation.

// Revision History 13 Dec 2003 Initial Version (Daniel Walker)

// next() and prior() are replacements for operator+ and operator- for
// non-random-access iterators. The semantics of these operators are
// such that after executing j = i + n, std::distance(i, j) equals
// n. Tests are provided to ensure next() has the same
// result. Parallel tests are provided for prior(). The tests call
// next() and prior() several times. next() and prior() are very
// simple functions, though, and it would be very strange if these
// tests were to fail.

#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

#include <list>
#include <vector>

#include <boost/next_prior.hpp>

template<class RandomAccessIterator, class ForwardIterator>
bool plus_one_test(RandomAccessIterator first, RandomAccessIterator last, ForwardIterator first2)
{
    RandomAccessIterator i = first;
    ForwardIterator j = first2;
    while(i != last)
        i = i + 1, j = boost::next(j);
    return std::distance(first, i) == std::distance(first2, j);
}

template<class RandomAccessIterator, class ForwardIterator>
bool plus_n_test(RandomAccessIterator first, RandomAccessIterator last, ForwardIterator first2)
{
    RandomAccessIterator i = first;
    ForwardIterator j = first2;
    for(int n = 0; i != last; ++n)
        i = first + n, j = boost::next(first2, n);
    return std::distance(first, i) == std::distance(first2, j);
}

template<class RandomAccessIterator, class BidirectionalIterator>
bool minus_one_test(RandomAccessIterator first, RandomAccessIterator last, BidirectionalIterator last2)
{
    RandomAccessIterator i = last;
    BidirectionalIterator j = last2;
    while(i != first)
        i = i - 1, j = boost::prior(j);
    return std::distance(i, last) == std::distance(j, last2);
}

template<class RandomAccessIterator, class BidirectionalIterator>
bool minus_n_test(RandomAccessIterator first, RandomAccessIterator last, BidirectionalIterator last2)
{
    RandomAccessIterator i = last;
    BidirectionalIterator j = last2;
    for(int n = 0; i != first; ++n)
        i = last - n, j = boost::prior(last2, n);
    return std::distance(i, last) == std::distance(j, last2);
}

int test_main(int, char*[])
{
    std::vector<int> x(8);
    std::list<int> y(x.begin(), x.end());

    BOOST_REQUIRE(plus_one_test(x.begin(), x.end(), y.begin()));
    BOOST_REQUIRE(plus_n_test(x.begin(), x.end(), y.begin()));
    BOOST_REQUIRE(minus_one_test(x.begin(), x.end(), y.end()));
    BOOST_REQUIRE(minus_n_test(x.begin(), x.end(), y.end()));
    return 0;
}
