// Ordered slots hello world example for Boost.Signals2
// Copyright Douglas Gregor 2001-2004.
// Copyright Frank Mori Hess 2009.
//
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// For more information, see http://www.boost.org

#include <iostream>
#include <boost/signals2/signal.hpp>

struct Hello
{
  void operator()() const
  {
    std::cout << "Hello";
  }
};

struct World
{
  void operator()() const
  {
    std::cout << ", World!" << std::endl;
  }
};

//[ good_morning_def_code_snippet
struct GoodMorning
{
  void operator()() const
  {
    std::cout << "... and good morning!" << std::endl;
  }
};
//]

int main()
{
//[ hello_world_ordered_code_snippet
  boost::signals2::signal<void ()> sig;

  sig.connect(1, World());  // connect with group 1
  sig.connect(0, Hello());  // connect with group 0
//]

//[ hello_world_ordered_invoke_code_snippet
  // by default slots are connected at the end of the slot list
  sig.connect(GoodMorning());

  // slots are invoked this order:
  // 1) ungrouped slots connected with boost::signals2::at_front
  // 2) grouped slots according to ordering of their groups
  // 3) ungrouped slots connected with boost::signals2::at_back
  sig();
//]

  return 0;
}

