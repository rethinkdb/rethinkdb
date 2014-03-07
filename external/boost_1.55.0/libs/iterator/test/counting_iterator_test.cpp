// (C) Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org for most recent version including documentation.
//
// Revision History
// 16 Feb 2001  Added a missing const. Made the tests run (somewhat) with
//              plain MSVC again. (David Abrahams)
// 11 Feb 2001  #if 0'd out use of counting_iterator on non-numeric types in
//              MSVC without STLport, so that the other tests may proceed
//              (David Abrahams)
// 04 Feb 2001  Added use of iterator_tests.hpp (David Abrahams)
// 28 Jan 2001  Removed not_an_iterator detritus (David Abrahams)
// 24 Jan 2001  Initial revision (David Abrahams)

#include <boost/config.hpp>

#ifdef __BORLANDC__     // Borland mis-detects our custom iterators
# pragma warn -8091     // template argument ForwardIterator passed to '...' is a output iterator
# pragma warn -8071     // Conversion may lose significant digits (due to counting_iterator<char> += n).
#endif

#ifdef BOOST_MSVC
# pragma warning(disable:4786) // identifier truncated in debug info
#endif

#include <boost/detail/iterator.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/new_iterator_tests.hpp>

#include <boost/next_prior.hpp>
#include <boost/mpl/if.hpp>
#include <boost/detail/iterator.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/limits.hpp>

#include <algorithm>
#include <climits>
#include <iterator>
#include <stdlib.h>
#ifndef __BORLANDC__
# include <boost/tuple/tuple.hpp>
#endif 
#include <vector>
#include <list>
#include <boost/detail/lightweight_test.hpp>
#ifndef BOOST_NO_SLIST
# ifdef BOOST_SLIST_HEADER
#   include BOOST_SLIST_HEADER
# else
# include <slist>
# endif
#endif


#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
template <class T>
struct signed_assert_nonnegative
{
    static void test(T x) { BOOST_TEST(x >= 0); }
};

template <class T>
struct unsigned_assert_nonnegative
{
    static void test(T x) {}
};

template <class T>
struct assert_nonnegative
  : boost::mpl::if_c<
        std::numeric_limits<T>::is_signed
      , signed_assert_nonnegative<T>
      , unsigned_assert_nonnegative<T>
    >::type
{
};
#endif

// Special tests for RandomAccess CountingIterators.
template <class CountingIterator, class Value>
void category_test(
    CountingIterator start,
    CountingIterator finish,
    Value,
    std::random_access_iterator_tag)
{
    typedef typename
        boost::detail::iterator_traits<CountingIterator>::difference_type
        difference_type;
    difference_type distance = boost::detail::distance(start, finish);

    // Pick a random position internal to the range
    difference_type offset = (unsigned)rand() % distance;
    
#ifdef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
    BOOST_TEST(offset >= 0);
#else 
    assert_nonnegative<difference_type>::test(offset);
#endif
    
    CountingIterator internal = start;
    std::advance(internal, offset);

    // Try some binary searches on the range to show that it's ordered
    BOOST_TEST(std::binary_search(start, finish, *internal));

    // #including tuple crashed borland, so I had to give up on tie().
    std::pair<CountingIterator,CountingIterator> xy(
        std::equal_range(start, finish, *internal));
    CountingIterator x = xy.first, y = xy.second;
    
    BOOST_TEST(boost::detail::distance(x, y) == 1);

    // Show that values outside the range can't be found
    BOOST_TEST(!std::binary_search(start, boost::prior(finish), *finish));

    // Do the generic random_access_iterator_test
    typedef typename CountingIterator::value_type value_type;
    std::vector<value_type> v;
    for (value_type z = *start; !(z == *finish); ++z)
        v.push_back(z);
    
    // Note that this test requires a that the first argument is
    // dereferenceable /and/ a valid iterator prior to the first argument
    boost::random_access_iterator_test(start, v.size(), v.begin());
}

// Special tests for bidirectional CountingIterators
template <class CountingIterator, class Value>
void category_test(CountingIterator start, Value v1, std::bidirectional_iterator_tag)
{
    Value v2 = v1;
    ++v2;

    // Note that this test requires a that the first argument is
    // dereferenceable /and/ a valid iterator prior to the first argument
    boost::bidirectional_iterator_test(start, v1, v2);
}

template <class CountingIterator, class Value>
void category_test(CountingIterator start, CountingIterator finish, Value v1, std::forward_iterator_tag)
{
    Value v2 = v1;
    ++v2;
    if (finish != start && finish != boost::next(start))
        boost::forward_readable_iterator_test(start, finish, v1, v2);
}

template <class CountingIterator, class Value>
void test_aux(CountingIterator start, CountingIterator finish, Value v1)
{
    typedef typename CountingIterator::iterator_category category;
    typedef typename CountingIterator::value_type value_type;

    // If it's a RandomAccessIterator we can do a few delicate tests
    category_test(start, finish, v1, category());

    // Okay, brute force...
    for (CountingIterator p = start
             ; p != finish && boost::next(p) != finish
             ; ++p)
    {
        BOOST_TEST(boost::next(*p) == *boost::next(p));
    }

    // prove that a reference can be formed to these values
    typedef typename CountingIterator::value_type value;
    const value* q = &*start;
    (void)q; // suppress unused variable warning
}

template <class Incrementable>
void test(Incrementable start, Incrementable finish)
{
    test_aux(boost::make_counting_iterator(start), boost::make_counting_iterator(finish), start);
}

template <class Integer>
void test_integer(Integer* = 0) // default arg works around MSVC bug
{
    Integer start = 0;
    Integer finish = 120;
    test(start, finish);
}

template <class Integer, class Category, class Difference>
void test_integer3(Integer* = 0, Category* = 0, Difference* = 0) // default arg works around MSVC bug
{
    Integer start = 0;
    Integer finish = 120;
    typedef boost::counting_iterator<Integer,Category,Difference> iterator;
    test_aux(iterator(start), iterator(finish), start);
}

template <class Container>
void test_container(Container* = 0)  // default arg works around MSVC bug
{
    Container c(1 + (unsigned)rand() % 1673);

    const typename Container::iterator start = c.begin();
    
    // back off by 1 to leave room for dereferenceable value at the end
    typename Container::iterator finish = start;
    std::advance(finish, c.size() - 1);
    
    test(start, finish);

    typedef typename Container::const_iterator const_iterator;
    test(const_iterator(start), const_iterator(finish));
}

class my_int1 {
public:
  my_int1() { }
  my_int1(int x) : m_int(x) { }
  my_int1& operator++() { ++m_int; return *this; }
  bool operator==(const my_int1& x) const { return m_int == x.m_int; }
private:
  int m_int;
};

class my_int2 {
public:
  typedef void value_type;
  typedef void pointer;
  typedef void reference;
  typedef std::ptrdiff_t difference_type;
  typedef std::bidirectional_iterator_tag iterator_category;

  my_int2() { }
  my_int2(int x) : m_int(x) { }
  my_int2& operator++() { ++m_int; return *this; }
  my_int2& operator--() { --m_int; return *this; }
  bool operator==(const my_int2& x) const { return m_int == x.m_int; }
private:
  int m_int;
};

class my_int3 {
public:
  typedef void value_type;
  typedef void pointer;
  typedef void reference;
  typedef std::ptrdiff_t difference_type;
  typedef std::random_access_iterator_tag iterator_category;

  my_int3() { }
  my_int3(int x) : m_int(x) { }
  my_int3& operator++() { ++m_int; return *this; }
  my_int3& operator+=(std::ptrdiff_t n) { m_int += n; return *this; }
  std::ptrdiff_t operator-(const my_int3& x) const { return m_int - x.m_int; }
  my_int3& operator--() { --m_int; return *this; }
  bool operator==(const my_int3& x) const { return m_int == x.m_int; }
  bool operator!=(const my_int3& x) const { return m_int != x.m_int; }
  bool operator<(const my_int3& x) const { return m_int < x.m_int; }
private:
  int m_int;
};

int main()
{
    // Test the built-in integer types.
    test_integer<char>();
    test_integer<unsigned char>();
    test_integer<signed char>();
    test_integer<wchar_t>();
    test_integer<short>();
    test_integer<unsigned short>();
    test_integer<int>();
    test_integer<unsigned int>();
    test_integer<long>();
    test_integer<unsigned long>();
#if defined(BOOST_HAS_LONG_LONG)
    test_integer< ::boost::long_long_type>();
    test_integer< ::boost::ulong_long_type>();
#endif

    // Test user-defined type.

    test_integer3<my_int1, std::forward_iterator_tag, int>();
    test_integer3<long, std::random_access_iterator_tag, int>();
    test_integer<my_int2>();
    test_integer<my_int3>();
    
   // Some tests on container iterators, to prove we handle a few different categories
    test_container<std::vector<int> >();
    test_container<std::list<int> >();
# ifndef BOOST_NO_SLIST
    test_container<BOOST_STD_EXTENSION_NAMESPACE::slist<int> >();
# endif
    
    // Also prove that we can handle raw pointers.
    int array[2000];
    test(boost::make_counting_iterator(array), boost::make_counting_iterator(array+2000-1));

    return boost::report_errors();
}
