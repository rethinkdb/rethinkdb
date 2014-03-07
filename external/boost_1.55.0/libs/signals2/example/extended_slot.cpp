// Example program for connecting an extended slot,
// using a signal's connect_extended and extended_slot_type.
//
// Copyright Frank Mori Hess 2009.
//
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// For more information, see http://www.boost.org

#include <boost/signals2/signal.hpp>
#include <iostream>
#include <string>

namespace bs2 = boost::signals2;

void single_shot_slot(const bs2::connection &conn, const std::string &message)
{
  conn.disconnect();
  std::cout << message;
}

int main()
{
  typedef bs2::signal<void (void)> sig_type;
  sig_type sig;
  {
    sig_type::extended_slot_type hello(&single_shot_slot, _1, "Hello");
    sig.connect_extended(hello);
  }
  sig();  // prints "Hello"
  {
    sig_type::extended_slot_type world(&single_shot_slot, _1, ", World!\n");
    sig.connect_extended(world);
  }
  sig();  // only prints ", World!\n" since hello slot has disconnected itself
  sig();  // prints nothing, world slot has disconnected itself

  return 0;
}

