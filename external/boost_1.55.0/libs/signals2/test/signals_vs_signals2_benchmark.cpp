/* Some performance benchmarks of boost.signals vs signals2 */

// Copyright Frank Mori Hess 2008.
// Distributed under the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/progress.hpp>
#include <boost/signals2.hpp>
#include <boost/signals.hpp>

typedef boost::signals2::signal<void (int),
  boost::signals2::optional_last_value<void>,
  int,
  std::less<int>,
  boost::function<void (int)>,
  boost::function<void (const boost::signals2::connection &, int)>,
  boost::signals2::dummy_mutex>
  new_signal_type;
typedef boost::signal<void (int)> old_signal_type;

class myslot
{
public:
  void operator()(int) {};
};

class trackable_slot: public myslot, public boost::signals::trackable
{};

template<typename Signal>
  void benchmark_invocation(unsigned num_connections)
{
  static const unsigned num_invocations = 1000000;

  Signal signal;
  std::cout << num_connections << " connections, invoking " << num_invocations << " times: ";
  unsigned n;
  for(n = 0; n < num_connections; ++n)
  {
    signal.connect(myslot());
  }
  {
    boost::progress_timer timer;
    unsigned i;
    for(i = 0; i < num_invocations; ++i)
    {
      signal(0);
    }
  }
}

void benchmark_old_tracked_invocation(unsigned num_connections)
{
  static const unsigned num_invocations = 1000000;

  old_signal_type signal;
  std::cout << "boost::signal, " << num_connections << " connections, tracking enabled, invoking " << num_invocations << " times: ";
  unsigned n;
  trackable_slot tslot;
  for(n = 0; n < num_connections; ++n)
  {
    signal.connect(tslot);
  }
  {
    boost::progress_timer timer;
    unsigned i;
    for(i = 0; i < num_invocations; ++i)
    {
      signal(0);
    }
  }
}

void benchmark_new_tracked_invocation(unsigned num_connections)
{
  static const unsigned num_invocations = 1000000;

  new_signal_type signal;
  std::cout << "boost::signals2::signal, " << num_connections << " connections, tracking enabled, invoking " << num_invocations << " times: ";
  boost::shared_ptr<int> trackable_ptr(new int(0));
  unsigned n;
  for(n = 0; n < num_connections; ++n)
  {
    new_signal_type::slot_type slot((myslot()));
    slot.track(trackable_ptr);
    signal.connect(slot);
  }
  {
    boost::progress_timer timer;
    unsigned i;
    for(i = 0; i < num_invocations; ++i)
    {
      signal(0);
    }
  }
}

template<typename Signal> class connection_type;
template<> class connection_type<old_signal_type>
{
public:
  typedef boost::signals::connection type;
};
template<> class connection_type<new_signal_type>
{
public:
  typedef boost::signals2::connection type;
};

template<typename Signal>
  void benchmark_connect_disconnect()
{
  static const unsigned num_connections = 1000000;
  std::vector<typename connection_type<Signal>::type> connections(num_connections);

  Signal signal;
  std::cout << "connecting " << num_connections << " connections then disconnecting: ";
  unsigned n;
  {
    boost::progress_timer timer;
    for(n = 0; n < num_connections; ++n)
    {
      connections.at(n) = signal.connect(myslot());
    }
    for(n = 0; n < num_connections; ++n)
    {
      connections.at(n).disconnect();
    }
  }
}

int main(int argc, const char **argv)
{
  std::cout << "\n";
  {
    std::cout << "boost::signals2::signal, ";
    benchmark_invocation<new_signal_type>(1);
  }
  {
    std::cout << "boost::signal, ";
    benchmark_invocation<old_signal_type>(1);
  }

  std::cout << "\n";
  {
    std::cout << "boost::signals2::signal, ";
    benchmark_invocation<new_signal_type>(10);
  }
  {
    std::cout << "boost::signal, ";
    benchmark_invocation<old_signal_type>(10);
  }

  std::cout << "\n";
  benchmark_new_tracked_invocation(1);
  benchmark_old_tracked_invocation(1);

  std::cout << "\n";
  benchmark_new_tracked_invocation(10);
  benchmark_old_tracked_invocation(10);

  std::cout << "\n";
  {
    std::cout << "boost::signals2::signal, ";
    benchmark_connect_disconnect<new_signal_type>();
  }
  {
    std::cout << "boost::signal, ";
    benchmark_connect_disconnect<old_signal_type>();
  }
}
