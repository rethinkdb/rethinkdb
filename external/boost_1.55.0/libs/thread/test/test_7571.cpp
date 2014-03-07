// Copyright (C) 2012 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2

#include <boost/date_time.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread_only.hpp>
#include <iostream>

// Number should be big enough to allow context switch between threads, otherwise the bug doesn't show.
static int MAX_COUNTS;

class ItemKeeper {

public:
  ItemKeeper() { }

  void doSomething() {
    boost::unique_lock<boost::mutex> scoped_lock(mutex);
    int counts = MAX_COUNTS;
    while (counts--);
  }

private:
  boost::mutex mutex;
};

ItemKeeper itemKeeper;

int MAX_ITERATIONS(5);

void threadFunc(int invokerID, bool& exceptionOccurred) {
  try {
    for (int i = 0; i < MAX_ITERATIONS; i++) {
      std::cout <<  "Thread " << invokerID << ", iteration " << i << std::endl;
      itemKeeper.doSomething();
    }
  } catch (...) {
    exceptionOccurred = true;
  }
}


int main(int argc, char* argv[]) {
  if (argc < 2) {
    MAX_COUNTS = 5000000;
  } else {
    std::string valueStr(argv[1]);
    bool has_only_digits = (valueStr.find_first_not_of( "0123456789" ) == std::string::npos);
    if (has_only_digits) {
      std::istringstream aStream(valueStr);
      aStream >> MAX_COUNTS;
    } else {
      std::cerr << "Argument should be an integer\n";
      return 1;
    }
  }

  bool exceptionOccurred1(false);
  bool exceptionOccurred2(false);

  boost::thread thread1(threadFunc, 1, boost::ref(exceptionOccurred1));
  boost::thread thread2(threadFunc, 2, boost::ref(exceptionOccurred2));

  boost::posix_time::time_duration timeout = boost::posix_time::milliseconds(10000*100);

  bool deadlockOccured(false);
  //thread1.join();
  //thread2.join();

  if (!thread1.timed_join(timeout)) {
    deadlockOccured = true;
    thread1.interrupt();
  }
  if (!thread2.timed_join(timeout)) {
    deadlockOccured = true;
    thread2.interrupt();
  }

  if (deadlockOccured) {
    std::cout << "Deadlock occurred\n";
    return 1;
  }
  if (exceptionOccurred1 || exceptionOccurred2) {
    std::cout << "Exception occurred\n";
    return 1;
  }
  return 0;
}

