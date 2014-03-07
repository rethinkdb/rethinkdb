// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_RELAXED_HEAP_DEBUG
#  define BOOST_RELAXED_HEAP_DEBUG 0
#endif

#include <boost/pending/relaxed_heap.hpp>
#include <boost/test/minimal.hpp>
#include <utility>
#include <iostream>
#include <vector>
#include <boost/optional.hpp>
#include <boost/random.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/progress.hpp>

typedef std::vector<boost::optional<double> > values_type;
values_type values;

struct less_extvalue
{
  typedef bool result_type;

  bool operator()(unsigned x, unsigned y) const
  {
    assert(values[x] && values[y]);
    return values[x] < values[y];
  }
};

using namespace std;

boost::optional<double> get_min_value()
{
  boost::optional<double> min_value;
  for (unsigned i = 0; i < values.size(); ++i) {
    if (values[i]) {
      if (!min_value || *values[i] < *min_value) min_value = values[i];
    }
  }
  return min_value;
}

void interactive_test()
{
  unsigned max_values;
  cout << "Enter max number of values in the heap> ";
  cin >> max_values;

  values.resize(max_values);
  boost::relaxed_heap<unsigned, less_extvalue> heap(max_values);

  char option;
  do {
    cout << "Enter command> ";
    if (cin >> option) {
      switch (option) {
      case 'i': case 'I':
        {
          unsigned index;
          double value;
          if (cin >> index >> value) {
            if (index >= values.size()) cout << "Index out of range.\n";
            else if (values[index]) cout << "Already in queue.\n";
            else {
              values[index] = value;
              heap.push(index);
              heap.dump_tree();
            }
          }
        }
        break;

      case 'u': case 'U':
        {
          unsigned index;
          double value;
          if (cin >> index >> value) {
            if (index >= values.size()) cout << "Index out of range.\n";
            else if (!values[index]) cout << "Not in queue.\n";
            else {
              values[index] = value;
              heap.update(index);
              heap.dump_tree();
            }
          }
        }
        break;

      case 'r': case 'R':
        {
          unsigned index;
          if (cin >> index) {
            if (index >= values.size()) cout << "Index out of range.\n";
            else if (!values[index]) cout << "Not in queue.\n";
            else {
              heap.remove(index);
              heap.dump_tree();
            }
          }
        }
        break;

      case 't': case 'T':
        {
          if (boost::optional<double> min_value = get_min_value()) {
            cout << "Top value is (" << heap.top() << ", "
                 << *values[heap.top()] << ").\n";
            BOOST_CHECK(*min_value == *values[heap.top()]);
          } else {
            cout << "Queue is empty.\n";
            BOOST_CHECK(heap.empty());
          }
        }
        break;

      case 'd': case 'D':
        {
          if (boost::optional<double> min_value = get_min_value()) {
            unsigned victim = heap.top();
            double value = *values[victim];
            cout << "Removed top value (" << victim << ", " << value << ")\n";
            BOOST_CHECK(*min_value == value);

            heap.pop();
            heap.dump_tree();
            values[victim].reset();
          } else {
            cout << "Queue is empty.\n";
            BOOST_CHECK(heap.empty());
          }
        }
        break;

      case 'q': case 'Q':
        break;

      default:
        cout << "Unknown command '" << option << "'.\n";
      }
    }
  } while (cin && option != 'q' && option != 'Q');
}

void random_test(int n, int iterations, int seed)
{
  values.resize(n);
  boost::relaxed_heap<unsigned, less_extvalue> heap(n);
  boost::minstd_rand gen(seed);
  boost::uniform_int<unsigned> rand_index(0, n-1);
  boost::uniform_real<> rand_value(-1000.0, 1000.0);
  boost::uniform_int<unsigned> which_option(0, 3);

  cout << n << std::endl;

#if BOOST_RELAXED_HEAP_DEBUG > 1
  heap.dump_tree();
#endif

  BOOST_REQUIRE(heap.valid());

#if BOOST_RELAXED_HEAP_DEBUG == 0
  boost::progress_display progress(iterations);
#endif

  for (int iteration = 0; iteration < iterations; ++iteration) {
#if BOOST_RELAXED_HEAP_DEBUG > 1
    std::cout << "Iteration #" << iteration << std::endl;
#endif
    unsigned victim = rand_index(gen);
    if (values[victim]) {
      switch (which_option(gen)) {
      case 0: case 3:
        {
          // Update with a smaller weight
          boost::uniform_real<> rand_smaller((rand_value.min)(), *values[victim]);
          values[victim] = rand_smaller(gen);
          assert(*values[victim] >= (rand_smaller.min)());
          assert(*values[victim] <= (rand_smaller.max)());

#if BOOST_RELAXED_HEAP_DEBUG > 0
          cout << "u " << victim << " " << *values[victim] << std::endl;
          cout.flush();
#endif
          heap.update(victim);
        }
        break;

      case 1:
        {
          // Delete minimum value in the queue.
          victim = heap.top();
          double top_value = *values[victim];
          BOOST_CHECK(*get_min_value() == top_value);
          if (*get_min_value() != top_value) return;
#if BOOST_RELAXED_HEAP_DEBUG > 0
          cout << "d" << std::endl;
          cout.flush();
#endif
          heap.pop();
          values[victim].reset();
#if BOOST_RELAXED_HEAP_DEBUG > 1
          cout << "(Removed " << victim << ")\n";
#endif // BOOST_RELAXED_HEAP_DEBUG > 1
        }
        break;

      case 2:
        {
          // Just remove this value from the queue completely
          values[victim].reset();
#if BOOST_RELAXED_HEAP_DEBUG > 0
          cout << "r " << victim << std::endl;
          cout.flush();
#endif
          heap.remove(victim);
        }
        break;

      default:
        cout << "Random number generator failed." << endl;
        BOOST_CHECK(false);
        return;
        break;
      }
    } else {
      values[victim] = rand_value(gen);
      assert(*values[victim] >= (rand_value.min)());
      assert(*values[victim] <= (rand_value.max)());

#if BOOST_RELAXED_HEAP_DEBUG > 0
      cout << "i " << victim << " " << *values[victim] << std::endl;
      cout.flush();
#endif
      heap.push(victim);
    }

#if BOOST_RELAXED_HEAP_DEBUG > 1
    heap.dump_tree();
#endif // BOOST_RELAXED_HEAP_DEBUG > 1

    BOOST_REQUIRE(heap.valid());

#if BOOST_RELAXED_HEAP_DEBUG == 0
    ++progress;
#endif
  }
}

int test_main(int argc, char* argv[])
{
  if (argc >= 3) {
    int n = boost::lexical_cast<int>(argv[1]);
    int iterations = boost::lexical_cast<int>(argv[2]);
    int seed = (argc >= 4? boost::lexical_cast<int>(argv[3]) : 1);
    random_test(n, iterations, seed);
  } else interactive_test();
  return 0;
}
