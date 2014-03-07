
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------
// See fast_mem_fn.hpp in this directory for details.

#include <vector>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <functional>

#include <boost/timer.hpp>
#include <boost/mem_fn.hpp>

#include "fast_mem_fn.hpp"

// test class that holds a single integer with getter function
class test
{
  int val_id;
public:

  explicit test(int id)
    : val_id(id)
  { }

  int id() const
  { return val_id; }

};

// STL style comparator that applies the CriterionExtractor function to both 
// operands and compares the results with Comparator
template<typename CriterionExtractor, typename Comparator>
class test_compare
{
  CriterionExtractor fnc_criterion;
  Comparator         fnc_compare;
public:

  explicit test_compare(CriterionExtractor criterion, Comparator compare)
    : fnc_criterion(criterion)
    , fnc_compare(compare)
  { }

  template<typename T>
  inline bool operator()(T const & lhs, T const & rhs) const
  {
    return fnc_compare(fnc_criterion(lhs),fnc_criterion(rhs));
  }
};

// helper function to construct an instance of the test_compare comparator.
template<typename CriterionExtractor, typename Comparator>
test_compare<CriterionExtractor,Comparator> 
make_test_compare(CriterionExtractor criterion, Comparator compare)
{
  return test_compare<CriterionExtractor,Comparator>(criterion,compare);
}

// the test case: sort N test objects by id
//
// the objects are in ascending order before the test run and in descending
// order after it

static const unsigned N = 2000000;

typedef std::vector<test> test_vector;


void setup_test(test_vector & v)
{
  v.clear();
  v.reserve(N);
  for (unsigned i = 0; i < N; ++i)
    v.push_back(test(i));
}

template<typename F> void do_test(test_vector & v, F criterion)
{
  std::sort(v.begin(),v.end(),make_test_compare(criterion,std::greater<int>()));
  assert(v.begin()->id() == N-1);
}


// compare performance with boost::mem_fn
int main()
{
  test_vector v;
  boost::timer t;
  double time1, time2;

  std::cout << 
      "Test case: sorting " << N << " objects.\n\n"
      "Criterion accessor called with | elasped seconds\n"
      "-------------------------------|----------------" << std::endl;

  setup_test(v);
  t.restart();
#if !BOOST_WORKAROUND(BOOST_MSVC, < 1400)
  do_test(v, BOOST_EXAMPLE_FAST_MEM_FN(& test::id));
#else // MSVC<8 does not like the implementation of the deduction macro:
  do_test(v, ::example::fast_mem_fn< int (test::*)() const, & test::id >());
#endif
  time1 = t.elapsed();
  std::cout << "fast_mem_fn                    | " << time1 << std::endl;

  setup_test(v);
  t.restart();
  do_test(v, boost::mem_fn(& test::id));
  time2 = t.elapsed();
  std::cout << "mem_fn                         | " << time2 << std::endl;

  std::cout << '\n' << (time2/time1-1)*100 << "% speedup" << std::endl;

  return 0;
}

