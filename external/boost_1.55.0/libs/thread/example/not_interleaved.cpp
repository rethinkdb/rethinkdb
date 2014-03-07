// (C) Copyright 2012 Howard Hinnant
// (C) Copyright 2012 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// adapted from the example given by Howard Hinnant in

#define BOOST_THREAD_VERSION 4

#include <iostream>
#include <boost/thread/scoped_thread.hpp>
#include <boost/thread/externally_locked_stream.hpp>

void use_cerr(boost::externally_locked_stream<std::ostream> &mcerr)
{
  using namespace boost;
  chrono::steady_clock::time_point tf = chrono::steady_clock::now() + chrono::seconds(10);
  while (chrono::steady_clock::now() < tf)
  {
    mcerr << "logging data to cerr\n";
    this_thread::sleep_for(chrono::milliseconds(500));
  }
}

void use_cout(boost::externally_locked_stream<std::ostream> &mcout)
{
  using namespace boost;
  chrono::steady_clock::time_point tf = chrono::steady_clock::now() + chrono::seconds(5);
  while (chrono::steady_clock::now() < tf)
  {
    mcout << "logging data to cout\n";
    this_thread::sleep_for(chrono::milliseconds(250));
  }
}

int main()
{
  using namespace boost;

  recursive_mutex terminal_mutex;

  externally_locked_stream<std::ostream> mcerr(std::cerr, terminal_mutex);
  externally_locked_stream<std::ostream> mcout(std::cout, terminal_mutex);
  externally_locked_stream<std::istream> mcin(std::cin, terminal_mutex);

  scoped_thread<> t1(boost::thread(use_cerr, boost::ref(mcerr)));
  scoped_thread<> t2(boost::thread(use_cout, boost::ref(mcout)));
  this_thread::sleep_for(chrono::seconds(2));
  std::string nm;
  {
    strict_lock<recursive_mutex> lk(terminal_mutex);
    std::ostream & gcout = mcout.get(lk);
    //std::istream & gcin = mcin.get(lk);
    gcout << "Enter name: ";
    //gcin >> nm;
  }
  t1.join();
  t2.join();
  mcout << nm << '\n';
  return 0;
}

