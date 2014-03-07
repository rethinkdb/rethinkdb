// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2013 Vicente J. Botet Escriba

#define BOOST_THREAD_PROVIDES_INTERRUPTIONS

#include <boost/thread/detail/config.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/latch.hpp>

#include <boost/detail/lightweight_test.hpp>
#include <vector>

namespace
{

  // Shared variables for generation latch test
  const int N_THREADS = 10;
  boost::latch gen_latch(N_THREADS);
  boost::mutex mutex;
  long global_parameter;

  void latch_thread()
  {
    {
      boost::unique_lock<boost::mutex> lock(mutex);
      global_parameter++;
    }
    gen_latch.count_down();
    //do something else
  }

} // namespace

void test_latch()
{
  boost::thread_group g;
  global_parameter = 0;

  try
  {
    for (int i = 0; i < N_THREADS; ++i)
      g.create_thread(&latch_thread);

    if (! gen_latch.try_wait())
      if (gen_latch.wait_for(boost::chrono::milliseconds(100)) ==  boost::cv_status::timeout)
        if (gen_latch.wait_until(boost::chrono::steady_clock::now()+boost::chrono::milliseconds(100)) ==  boost::cv_status::timeout)
          gen_latch.wait(); // All the threads have been updated the global_parameter
    BOOST_TEST_EQ(global_parameter, N_THREADS);

    g.join_all();
  }
  catch (...)
  {
    g.interrupt_all();
    g.join_all();
    throw;
  }

}

int main()
{
  test_latch();
  return boost::report_errors();
}

