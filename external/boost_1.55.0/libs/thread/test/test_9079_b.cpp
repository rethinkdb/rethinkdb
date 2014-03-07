// Copyright (C) 2013 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// B

#include <boost/atomic.hpp>
//#include <boost/log/trivial.hpp>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>

typedef boost::chrono::high_resolution_clock Clock;
typedef Clock::time_point TimePoint;

inline TimePoint real_time_now()
{
  return Clock::now();
}

class Foo {
  boost::atomic<bool> m_is_exiting;
  TimePoint m_next_tick_time;

public:

  bool is_exiting() const
  {
    return m_is_exiting;
  }

  TimePoint spawn_tasks() // note that in my app, this call takes more time than here
  {
    using namespace boost::chrono;
    const TimePoint now = real_time_now();

    if (m_next_tick_time < now) {
      m_next_tick_time = now + seconds(1);
      //BOOST_LOG_TRIVIAL(info) << "TICK!";
    }

    return m_next_tick_time;
  }

};

int main()
{
  using namespace boost::chrono;
  static const milliseconds MIN_TIME_TASKS_SPAWN_FREQUENCY = milliseconds(1);
  //microseconds(1); // THE SHORTER THE QUICKER TO REPRODUCE THE BUG

  boost::condition_variable m_task_spawn_condition;
  Foo foo;

  boost::mutex main_thread_mutex;
  boost::unique_lock < boost::mutex > main_thread_lock(main_thread_mutex);

  //BOOST_LOG_TRIVIAL(info) << "[TaskScheduler::run_and_wait] Scheduling loop - BEGIN";

  while (!foo.is_exiting()) {
    const TimePoint next_task_spawn_time = foo.spawn_tasks();

    const TimePoint now = real_time_now();
    const TimePoint next_minimum_spawn_time = now + MIN_TIME_TASKS_SPAWN_FREQUENCY;
    const TimePoint next_spawn_time = next_task_spawn_time > TimePoint()
        && next_task_spawn_time < next_minimum_spawn_time
        ? next_task_spawn_time : next_minimum_spawn_time;

    const TimePoint::duration wait_time = next_spawn_time - now;
    if (wait_time > wait_time.zero()) {
      // BOOST_LOG_TRIVIAL(trace) << "WAIT TIME: " << wait_time; // UNCOMMENT THIS: MAKES IT WORKS. WAT??????
      m_task_spawn_condition.wait_until(
          main_thread_lock,
          next_spawn_time); // DON'T WORK: WILL WAIT IF next_spawn_time is too close!
    }

  }

}
