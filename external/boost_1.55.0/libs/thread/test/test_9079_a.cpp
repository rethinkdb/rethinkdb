// Copyright (C) 2013 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A

//#include <boost/log/trivial.hpp>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>

//#if !defined(BOOST_NO_CXX11_ALIGNAS)
//#error
//#  define BOOST_ALIGNMENT2(x) alignas(x)
//#elif defined(_MSC_VER)
//#error
//#  define BOOST_ALIGNMENT2(x) __declspec(align(x))
//#elif defined(__GNUC__)
//#error
//#  define BOOST_ALIGNMENT(x) __attribute__ ((__aligned__(x)))
//#else
//#error
//#  define BOOST_NO_ALIGNMENT2
//#  define BOOST_ALIGNMENT2(x)
//#endif

typedef boost::chrono::high_resolution_clock Clock;
typedef Clock::time_point TimePoint;

inline TimePoint real_time_now()
{
  return Clock::now();
}

int main()
{
  using namespace boost::chrono;

  boost::condition_variable m_task_spawn_condition;

  boost::mutex main_thread_mutex;
  boost::unique_lock < boost::mutex > main_thread_lock(main_thread_mutex);

  //BOOST_LOG_TRIVIAL(info) << "[TaskScheduler::run_and_wait] Scheduling loop - BEGIN";

  //while (true)
  {
    static const milliseconds TIME_BACK = milliseconds(1);
    m_task_spawn_condition.wait_until(
        main_thread_lock,
        real_time_now() - TIME_BACK); // wait forever
    m_task_spawn_condition.wait_for( main_thread_lock, - TIME_BACK ); // same problem
    //BOOST_LOG_TRIVIAL(trace) << "TICK";
  }

}
