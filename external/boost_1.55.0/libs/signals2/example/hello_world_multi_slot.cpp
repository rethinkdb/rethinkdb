// Multiple slot hello world example for Boost.Signals2
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

//[ hello_def_code_snippet
struct Hello
{
  void operator()() const
  {
    std::cout << "Hello";
  }
};
//]

//[ world_def_code_snippet
struct World
{
  void operator()() const
  {
    std::cout << ", World!" << std::endl;
  }
};
//]

int main()
{
//[ hello_world_multi_code_snippet
  boost::signals2::signal<void ()> sig;

  sig.connect(Hello());
  sig.connect(World());

  sig();
//]

  return 0;
}

