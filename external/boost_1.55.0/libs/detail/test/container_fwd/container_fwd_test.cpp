
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/container_fwd.hpp>

#if BOOST_WORKAROUND(__GNUC__, < 3) && \
    !defined(__SGI_STL_PORT) && !defined(_STLPORT_VERSION)
template <class charT, class Allocator>
static void test(
    std::basic_string<charT, std::string_char_traits<charT>, Allocator> const&)
{
}
#else
template <class charT, class Allocator>
static void test(
    std::basic_string<charT, std::char_traits<charT>, Allocator> const&)
{
}
#endif
    
template <class T, class Allocator>
static void test(std::deque<T, Allocator> const&)
{
}

template <class T, class Allocator>
static void test(std::list<T, Allocator> const&)
{
}

template <class T, class Allocator>
static void test(std::vector<T, Allocator> const&)
{
}

template <class Key, class T, class Compare, class Allocator>
static void test(std::map<Key, T, Compare, Allocator> const&)
{
}

template <class Key, class T, class Compare, class Allocator>
static void test(std::multimap<Key, T, Compare, Allocator> const&)
{
}

template <class Key, class Compare, class Allocator>
static void test(std::set<Key, Compare, Allocator> const&)
{
}

template <class Key, class Compare, class Allocator>
static void test(std::multiset<Key, Compare, Allocator> const&)
{
}

template <std::size_t N>
static void test(std::bitset<N> const&)
{
}

template <class T>
static void test(std::complex<T> const&)
{
}

template <class X, class Y>
static void test(std::pair<X, Y> const&)
{
}

#include <deque>
#include <list>
#include <vector>
#include <map>
#include <set>
#include <bitset>
#include <string>
#include <complex>
#include <utility>

int main()
{
    std::deque<int> x1;
    std::list<std::string> x2;
    std::vector<float> x3;
    std::vector<bool> x4;
    std::map<int, int> x5;
    std::multimap<float, int*> x6;
    std::set<std::string> x7;
    std::multiset<std::vector<int> > x8;
    std::bitset<10> x9;
    std::string x10;
    std::complex<double> x11;
    std::pair<std::list<int>, char***> x12;

    test(x1);
    test(x2);
    test(x3);
    test(x4);
    test(x5);
    test(x6);
    test(x7);
    test(x8);
    test(x9);
    test(x10);
    test(x11);
    test(x12);

    return 0;
}
