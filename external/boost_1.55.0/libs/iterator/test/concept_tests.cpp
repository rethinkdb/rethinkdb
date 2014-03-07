// (C) Copyright Jeremy Siek 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/iterator/iterator_concepts.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/operators.hpp>

struct new_random_access
  : std::random_access_iterator_tag
  , boost::random_access_traversal_tag
{};

struct new_iterator
  : public boost::iterator< new_random_access, int >
{
  int& operator*() const { return *m_x; }
  new_iterator& operator++() { return *this; }
  new_iterator operator++(int) { return *this; }
  new_iterator& operator--() { return *this; }
  new_iterator operator--(int) { return *this; }
  new_iterator& operator+=(std::ptrdiff_t) { return *this; }
  new_iterator operator+(std::ptrdiff_t) { return *this; }
  new_iterator& operator-=(std::ptrdiff_t) { return *this; }
  std::ptrdiff_t operator-(const new_iterator&) const { return 0; }
  new_iterator operator-(std::ptrdiff_t) const { return *this; }
  bool operator==(const new_iterator&) const { return false; }
  bool operator!=(const new_iterator&) const { return false; }
  bool operator<(const new_iterator&) const { return false; }
  int* m_x;
};
new_iterator operator+(std::ptrdiff_t, new_iterator x) { return x; }

struct old_iterator
  : public boost::iterator<std::random_access_iterator_tag, int>
{
  int& operator*() const { return *m_x; }
  old_iterator& operator++() { return *this; }
  old_iterator operator++(int) { return *this; }
  old_iterator& operator--() { return *this; }
  old_iterator operator--(int) { return *this; }
  old_iterator& operator+=(std::ptrdiff_t) { return *this; }
  old_iterator operator+(std::ptrdiff_t) { return *this; }
  old_iterator& operator-=(std::ptrdiff_t) { return *this; }
  old_iterator operator-(std::ptrdiff_t) const { return *this; }
  std::ptrdiff_t operator-(const old_iterator&) const { return 0; }
  bool operator==(const old_iterator&) const { return false; }
  bool operator!=(const old_iterator&) const { return false; }
  bool operator<(const old_iterator&) const { return false; }
  int* m_x;
};
old_iterator operator+(std::ptrdiff_t, old_iterator x) { return x; }

int
main()
{
  boost::iterator_traversal<new_iterator>::type tc;
  boost::random_access_traversal_tag derived = tc;
  (void)derived;
  
  boost::function_requires<
    boost_concepts::WritableIteratorConcept<int*> >();
  boost::function_requires<
    boost_concepts::LvalueIteratorConcept<int*> >();
  boost::function_requires<
    boost_concepts::RandomAccessTraversalConcept<int*> >();

  boost::function_requires<
    boost_concepts::ReadableIteratorConcept<const int*> >();
  boost::function_requires<
    boost_concepts::LvalueIteratorConcept<const int*> >();
  boost::function_requires<
    boost_concepts::RandomAccessTraversalConcept<const int*> >();

  boost::function_requires<
    boost_concepts::WritableIteratorConcept<new_iterator> >();
  boost::function_requires<
    boost_concepts::LvalueIteratorConcept<new_iterator> >();
  boost::function_requires<
    boost_concepts::RandomAccessTraversalConcept<new_iterator> >();

  boost::function_requires<
    boost_concepts::WritableIteratorConcept<old_iterator> >();
  boost::function_requires<
    boost_concepts::LvalueIteratorConcept<old_iterator> >();
  boost::function_requires<
    boost_concepts::RandomAccessTraversalConcept<old_iterator> >();

  boost::function_requires<
    boost_concepts::InteroperableIteratorConcept<int*, int const*> >();

  return 0;
}
