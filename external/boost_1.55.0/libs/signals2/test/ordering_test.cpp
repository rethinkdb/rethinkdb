// Boost.Signals2 library

// Copyright Douglas Gregor 2002-2004. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <boost/test/minimal.hpp>
#include <boost/signals2.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <functional>

std::vector<int> valuesOutput;
bool ungrouped1 = false;
bool ungrouped2 = false;
bool ungrouped3 = false;

struct emit_int {
  emit_int(int v) : value(v) {}

  void operator()() const
  {
    BOOST_CHECK(value == 42 || (!ungrouped1 && !ungrouped2 && !ungrouped3));
    valuesOutput.push_back(value);
    std::cout << value << ' ';
  }

private:
  int value;
};

struct write_ungrouped1 {
  void operator()() const
  {
    BOOST_CHECK(!ungrouped1);
    ungrouped1 = true;
    std::cout << "(Ungrouped #1)" << ' ';
  }
};

struct write_ungrouped2 {
  void operator()() const
  {
    BOOST_CHECK(!ungrouped2);
    ungrouped2 = true;
    std::cout << "(Ungrouped #2)" << ' ';
  }
};

struct write_ungrouped3 {
  void operator()() const
  {
    BOOST_CHECK(!ungrouped3);
    ungrouped3 = true;
    std::cout << "(Ungrouped #3)" << ' ';
  }
};

int return_argument(int x)
{
  return x;
}

void test_group_compare()
{
  boost::signals2::signal
  <
    int (),
    boost::signals2::last_value<int>,
    int,
    std::greater< int >
  > sig;

  sig.connect( 1, boost::bind( &return_argument, 1) );
  sig.connect( 2, boost::bind( &return_argument, 2) );

  BOOST_CHECK(sig() == 1);
}

int test_main(int, char* [])
{
  using namespace std;
  srand(time(0));

  std::vector<int> sortedValues;

  boost::signals2::signal<void ()> sig;
  sig.connect(write_ungrouped1());
  for (int i = 0; i < 100; ++i) {
#ifdef BOOST_NO_STDC_NAMESPACE
    int v = rand() % 100;
#else
    int v = std::rand() % 100;
#endif
    sortedValues.push_back(v);
    sig.connect(v, emit_int(v));

    if (i == 50) {
      sig.connect(write_ungrouped2());
    }
  }
  sig.connect(write_ungrouped3());

  std::sort(sortedValues.begin(), sortedValues.end());

  // 17 at beginning, 42 at end
  sortedValues.insert(sortedValues.begin(), 17);
  sig.connect(emit_int(17), boost::signals2::at_front);
  sortedValues.push_back(42);
  sig.connect(emit_int(42));

  sig();
  std::cout << std::endl;

  BOOST_CHECK(valuesOutput == sortedValues);
  BOOST_CHECK(ungrouped1);
  BOOST_CHECK(ungrouped2);
  BOOST_CHECK(ungrouped3);

  test_group_compare();

  return 0;
}
