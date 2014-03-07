/* multi-threaded signal invocation benchmark */

// Copyright Frank Mori Hess 2007-2008.
// Distributed under the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/signals2.hpp>
#include <boost/thread/thread.hpp>

typedef boost::signals2::signal<void ()> signal_type;

void myslot()
{
/*  std::cout << __FUNCTION__ << std::endl;
  sleep(1);*/
}

void thread_initial(signal_type *signal, unsigned num_invocations)
{
  unsigned i;
  for(i = 0; i < num_invocations; ++i)
  {
    (*signal)();
  }
}

int main(int argc, const char **argv)
{
  if(argc < 3)
  {
    std::cerr << "usage: " << argv[0] << " <num threads> <num connections>" << std::endl;
    return -1;
  }
  static const unsigned num_threads = std::strtol(argv[1], 0, 0);
  static const unsigned num_connections = std::strtol(argv[2], 0, 0);
  boost::thread_group threads;
  signal_type sig;

  std::cout << "Connecting " << num_connections << " connections to signal.\n";
  unsigned i;
  for(i = 0; i < num_connections; ++i)
  {
    sig.connect(&myslot);
  }
  const unsigned num_slot_invocations = 1000000;
  const unsigned signal_invocations_per_thread = num_slot_invocations / (num_threads * num_connections);
  std::cout << "Launching " << num_threads << " thread(s) to invoke signal " << signal_invocations_per_thread << " times per thread.\n";
  for(i = 0; i < num_threads; ++i)
  {
    threads.create_thread(boost::bind(&thread_initial, &sig, signal_invocations_per_thread));
  }
  threads.join_all();
  return 0;
}
