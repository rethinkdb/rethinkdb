//  (C) Copyright Thomas Witt 2003.
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

#include <boost/config.hpp>
#include <iostream>

#include <algorithm>
#include <functional>
#include <numeric>

#include <boost/iterator/iterator_adaptor.hpp>
#if !BOOST_WORKAROUND(__MWERKS__, <= 0x2407)
# include <boost/iterator/is_readable_iterator.hpp>
# include <boost/iterator/is_lvalue_iterator.hpp>
#endif 
#include <boost/pending/iterator_tests.hpp>

# include <boost/type_traits/broken_compiler_spec.hpp>

# include <boost/detail/lightweight_test.hpp>

#include <stdlib.h>
#include <vector>
#include <deque>
#include <set>
#include <list>

#include "static_assert_same.hpp"

#include <boost/iterator/detail/config_def.hpp>

using boost::dummyT;

struct mult_functor {
  typedef int result_type;
  typedef int argument_type;
  // Functors used with transform_iterator must be
  // DefaultConstructible, as the transform_iterator must be
  // DefaultConstructible to satisfy the requirements for
  // TrivialIterator.
  mult_functor() { }
  mult_functor(int aa) : a(aa) { }
  int operator()(int b) const { return a * b; }
  int a;
};

template <class Pair>
struct select1st_ 
  : public std::unary_function<Pair, typename Pair::first_type>
{
  const typename Pair::first_type& operator()(const Pair& x) const {
    return x.first;
  }
  typename Pair::first_type& operator()(Pair& x) const {
    return x.first;
  }
};

struct one_or_four {
  bool operator()(dummyT x) const {
    return x.foo() == 1 || x.foo() == 4;
  }
};

typedef std::deque<int> storage;
typedef std::deque<int*> pointer_deque;
typedef std::set<storage::iterator> iterator_set;

template <class T> struct foo;

void blah(int) { }

struct my_gen
{
  typedef int result_type;
  my_gen() : n(0) { }
  int operator()() { return ++n; }
  int n;
};

template <class V>
struct ptr_iterator
  : boost::iterator_adaptor<
        ptr_iterator<V>
      , V*
      , V
      , boost::random_access_traversal_tag
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x551))
      , V&
#endif 
   >
{
private:
  typedef boost::iterator_adaptor<
        ptr_iterator<V>
      , V*
      , V
      , boost::random_access_traversal_tag
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x551))
      , V&
#endif 
    > super_t;
    
public:
  ptr_iterator() { }
  ptr_iterator(V* d) : super_t(d) { }

  template <class V2>
  ptr_iterator(
        const ptr_iterator<V2>& x
      , typename boost::enable_if_convertible<V2*, V*>::type* = 0
  )
    : super_t(x.base())
  {}
};

// Non-functional iterator for category modification checking
template <class Iter, class Traversal>
struct modify_traversal
  :  boost::iterator_adaptor<
         modify_traversal<Iter, Traversal>
       , Iter
       , boost::use_default
       , Traversal
     >
{};
  
template <class T>
struct fwd_iterator
  : boost::iterator_adaptor<
        fwd_iterator<T>
      , boost::forward_iterator_archetype<T>
    >
{
private:
  typedef boost::iterator_adaptor<
        fwd_iterator<T>
      , boost::forward_iterator_archetype<T>
  > super_t;
    
public:
  fwd_iterator() { }
  fwd_iterator(boost::forward_iterator_archetype<T> d) : super_t(d) { }
};

template <class T>
struct in_iterator
  : boost::iterator_adaptor<
        in_iterator<T>
      , boost::input_iterator_archetype_no_proxy<T>
    >
{
private:
  typedef boost::iterator_adaptor<
        in_iterator<T>
      , boost::input_iterator_archetype_no_proxy<T>
  > super_t;
    
public:
  in_iterator() { }
  in_iterator(boost::input_iterator_archetype_no_proxy<T> d) : super_t(d) { }
};

template <class Iter>
struct constant_iterator
  : boost::iterator_adaptor<
        constant_iterator<Iter>
      , Iter
      , typename std::iterator_traits<Iter>::value_type const
    >
{
    typedef boost::iterator_adaptor<
        constant_iterator<Iter>
      , Iter
      , typename std::iterator_traits<Iter>::value_type const
    > base_t;
    
  constant_iterator() {}
  constant_iterator(Iter it)
    : base_t(it) {}
};

char (& traversal2(boost::incrementable_traversal_tag) )[1];
char (& traversal2(boost::single_pass_traversal_tag  ) )[2];
char (& traversal2(boost::forward_traversal_tag      ) )[3];
char (& traversal2(boost::bidirectional_traversal_tag) )[4];
char (& traversal2(boost::random_access_traversal_tag) )[5];

template <class Cat>
struct traversal3
{
    static typename boost::iterator_category_to_traversal<Cat>::type x;
    BOOST_STATIC_CONSTANT(std::size_t, value = sizeof(traversal2(x)));
    typedef char (&type)[value];
};

template <class Cat>
typename traversal3<Cat>::type traversal(Cat);

template <class Iter, class Trav>
int static_assert_traversal(Iter* = 0, Trav* = 0)
{
    typedef typename boost::iterator_category_to_traversal<
        BOOST_DEDUCED_TYPENAME Iter::iterator_category
        >::type t2;

    return static_assert_same<Trav,t2>::value;
}

int
main()
{
  dummyT array[] = { dummyT(0), dummyT(1), dummyT(2), 
                     dummyT(3), dummyT(4), dummyT(5) };
  const int N = sizeof(array)/sizeof(dummyT);

  // sanity check, if this doesn't pass the test is buggy
  boost::random_access_iterator_test(array, N, array);

  // Test the iterator_adaptor
  {
    ptr_iterator<dummyT> i(array);
    boost::random_access_iterator_test(i, N, array);
    
    ptr_iterator<const dummyT> j(array);
    boost::random_access_iterator_test(j, N, array);
    boost::const_nonconst_iterator_test(i, ++j);
  }

  int test;
  // Test the iterator_traits
  {
    // Test computation of defaults
    typedef ptr_iterator<int> Iter1;
    // don't use std::iterator_traits here to avoid VC++ problems
    test = static_assert_same<Iter1::value_type, int>::value;
    test = static_assert_same<Iter1::reference, int&>::value;
    test = static_assert_same<Iter1::pointer, int*>::value;
    test = static_assert_same<Iter1::difference_type, std::ptrdiff_t>::value;
#if !BOOST_WORKAROUND(__MWERKS__, <= 0x2407)
    BOOST_STATIC_ASSERT((boost::is_convertible<Iter1::iterator_category, std::random_access_iterator_tag>::value));
#endif 
  }
  
  {  
    // Test computation of default when the Value is const
    typedef ptr_iterator<int const> Iter1;
    test = static_assert_same<Iter1::value_type, int>::value;
    test = static_assert_same<Iter1::reference, const int&>::value;
    
#if !BOOST_WORKAROUND(__MWERKS__, <= 0x2407)
    BOOST_STATIC_ASSERT(boost::is_readable_iterator<Iter1>::value);
# ifndef BOOST_NO_LVALUE_RETURN_DETECTION
    BOOST_STATIC_ASSERT(boost::is_lvalue_iterator<Iter1>::value);
# endif 
#endif

#if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564)) // borland drops constness
    test = static_assert_same<Iter1::pointer, int const*>::value;
#endif 
  }

  {
    // Test constant iterator idiom
    typedef ptr_iterator<int> BaseIter;
    typedef constant_iterator<BaseIter> Iter;

    test = static_assert_same<Iter::value_type, int>::value;
    test = static_assert_same<Iter::reference, int const&>::value;
#if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564)) // borland drops constness
    test = static_assert_same<Iter::pointer, int const*>::value;
#endif 

#ifndef BOOST_NO_LVALUE_RETURN_DETECTION
    BOOST_STATIC_ASSERT(boost::is_non_const_lvalue_iterator<BaseIter>::value);
    BOOST_STATIC_ASSERT(boost::is_lvalue_iterator<Iter>::value);
#endif 
    
    typedef modify_traversal<BaseIter, boost::incrementable_traversal_tag> IncrementableIter;

    static_assert_traversal<BaseIter,boost::random_access_traversal_tag>();
    static_assert_traversal<IncrementableIter,boost::incrementable_traversal_tag>();
  }
  
  // Test the iterator_adaptor
  {
    ptr_iterator<dummyT> i(array);
    boost::random_access_iterator_test(i, N, array);
    
    ptr_iterator<const dummyT> j(array);
    boost::random_access_iterator_test(j, N, array);
    boost::const_nonconst_iterator_test(i, ++j);
  }

  // check operator-> with a forward iterator
  {
    boost::forward_iterator_archetype<dummyT> forward_iter;

    typedef fwd_iterator<dummyT> adaptor_type;

    adaptor_type i(forward_iter);
    int zero = 0;
    if (zero) // don't do this, just make sure it compiles
      BOOST_TEST((*i).m_x == i->foo());      
  }
  
  // check operator-> with an input iterator
  {
    boost::input_iterator_archetype_no_proxy<dummyT> input_iter;
    typedef in_iterator<dummyT> adaptor_type;
    adaptor_type i(input_iter);
    int zero = 0;
    if (zero) // don't do this, just make sure it compiles
      BOOST_TEST((*i).m_x == i->foo());      
  }

  // check that base_type is correct
  {
    // Test constant iterator idiom
    typedef ptr_iterator<int> BaseIter;

    test = static_assert_same<BaseIter::base_type,int*>::value;
    test = static_assert_same<constant_iterator<BaseIter>::base_type,BaseIter>::value;

    typedef modify_traversal<BaseIter, boost::incrementable_traversal_tag> IncrementableIter;

    test = static_assert_same<IncrementableIter::base_type,BaseIter>::value;
  }

  std::cout << "test successful " << std::endl;
  (void)test;
  return boost::report_errors();
}
