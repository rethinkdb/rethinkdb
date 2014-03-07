//  (C) Copyright 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// This performance test is based on the performance test provided by maxim.yegorushkin
// at https://svn.boost.org/trac/boost/ticket/7422

#define BOOST_THREAD_DONT_PROVIDE_INTERRUPTIONS

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/chrono/stopwatches/simple_stopwatch.hpp>

#include <condition_variable>
#include <future>
#include <limits>
#include <cstdio>
#include <thread>
#include <mutex>
#include <vector>

////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{
  ////////////////////////////////////////////////////////////////////////////////////////////////

//  class Stopwatch
//  {
//  public:
//    typedef long long rep;
//
//    static rep now()
//    {
//      timespec ts;
//      if (clock_gettime(CLOCK_MONOTONIC, &ts)) abort();
//      return ts.tv_sec * rep(1000000000) + ts.tv_nsec;
//    }
//
//    Stopwatch() :
//      start_(now())
//    {
//    }
//
//    rep elapsed() const
//    {
//      return now() - start_;
//    }
//
//  private:
//    rep start_;
//  };

  typedef boost::chrono::simple_stopwatch<> Stopwatch;
  ////////////////////////////////////////////////////////////////////////////////////////////////

  struct BoostTypes
  {
    typedef boost::condition_variable condition_variable;
    typedef boost::mutex mutex;
    typedef boost::mutex::scoped_lock scoped_lock;
  };

  struct StdTypes
  {
    typedef std::condition_variable condition_variable;
    typedef std::mutex mutex;
    typedef std::unique_lock<std::mutex> scoped_lock;
  };

  template <class Types>
  struct SharedData: Types
  {
    unsigned const iterations;
    unsigned counter;
    unsigned semaphore;
    typename Types::condition_variable cnd;
    typename Types::mutex mtx;
    Stopwatch::rep producer_time;

    SharedData(unsigned iterations, unsigned consumers) :
      iterations(iterations), counter(), semaphore(consumers) // Initialize to the number of consumers. (*)
          , producer_time()
    {
    }
  };


  ////////////////////////////////////////////////////////////////////////////////////////////////

  template <class S>
  void producer_thread(S* shared_data)
  {
    Stopwatch sw;

    unsigned const consumers = shared_data->semaphore; // (*)
    for (unsigned i = shared_data->iterations; i--;)
    {
      {
        typename S::scoped_lock lock(shared_data->mtx);
        // Wait till all consumers signal.
        while (consumers != shared_data->semaphore)
        {
          shared_data->cnd.wait(lock);
        }
        shared_data->semaphore = 0;
        // Signal consumers.
        ++shared_data->counter;
      }
      shared_data->cnd.notify_all();
    }

    shared_data->producer_time = sw.elapsed().count();
  }

  template <class S>
  void consumer_thread(S* shared_data)
  {
    unsigned counter = 0;
    while (counter != shared_data->iterations)
    {
      {
        typename S::scoped_lock lock(shared_data->mtx);
        // Wait till the producer signals.
        while (counter == shared_data->counter)
        {
          shared_data->cnd.wait(lock);
        }
        counter = shared_data->counter;
        // Signal the producer.
        ++shared_data->semaphore;
      }
      shared_data->cnd.notify_all();
    }
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////

  template <class Types>
  Stopwatch::rep benchmark_ping_pong(unsigned consumer_count)
  {
    typedef SharedData<Types> S;

    auto best_producer_time = std::numeric_limits<Stopwatch::rep>::max BOOST_PREVENT_MACRO_SUBSTITUTION ();

    std::vector<std::thread> consumers
    { consumer_count };

    // Run the benchmark 10 times and report the best time.
    for (int times = 10; times--;)
    {
      S shared_data
      { 100000, consumer_count };

      // Start the consumers.
      for (unsigned i = 0; i < consumer_count; ++i)
        consumers[i] = std::thread
        { consumer_thread<S> , &shared_data };
      // Start the producer and wait till it finishes.
      std::thread
      { producer_thread<S> , &shared_data }.join();
      // Wait till consumers finish.
      for (unsigned i = 0; i < consumer_count; ++i)
        consumers[i].join();

      best_producer_time = std::min BOOST_PREVENT_MACRO_SUBSTITUTION (best_producer_time, shared_data.producer_time);

    }

    return best_producer_time;
  }

////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////

// sudo chrt -f 99 /usr/bin/time -f "\n***\ntime: %E\ncontext switches: %c\nwaits: %w" /home/max/otsquant/build/Linux-x86_64-64.g++-release/test/test

/*

 Producer-consumer ping-pong tests. It aims to benchmark condition variables with and without
 thread cancellation support by comparing the time it took to complete the benchmark.

 Condition variable with thread cancellation support is boost::condition_variable from
 boost-1.51. Without - std::condition_variable that comes with gcc-4.7.2.

 One producer, one to CONSUMER_MAX consumers. The benchmark calls
 condition_variable::notify_all() without holding a mutex to maximize contention within this
 function. Each benchmark for a number of consumers is run three times and the best time is
 picked to get rid of outliers.

 The results are reported for each benchmark for a number of consumers. The most important number
 is (std - boost) / std * 100. Positive numbers are when boost::condition_variable is faster,
 negative it is slower.

 */

int main()
{
  std::printf("MAIN\n");
  enum
  {
    CONSUMER_MAX = 2
  };

  struct
  {
    Stopwatch::rep boost, std;
  } best_times[CONSUMER_MAX] = {};

  for (unsigned i = 1; i <= CONSUMER_MAX; ++i)
  {
    auto& b = best_times[i - 1];
    std::printf("STD: %d\n", i);
    b.std = benchmark_ping_pong<StdTypes> (i);
    std::printf("BOOST: %d\n", i);
    b.boost = benchmark_ping_pong<BoostTypes> (i);

    std::printf("consumers:                 %4d\n", i);
    std::printf("best std producer time:   %15.9fsec\n", b.std * 1e-9);
    std::printf("best boost producer time: %15.9fsec\n", b.boost * 1e-9);
    std::printf("(std - boost) / std:       %7.2f%%\n", (b.std - b.boost) * 100. / b.std);
  }

  printf("\ncsv:\n\n");
  printf("consumers,(std-boost)/std,std,boost\n");
  for (unsigned i = 1; i <= CONSUMER_MAX; ++i)
  {
    auto& b = best_times[i - 1];
    printf("%d,%f,%lld,%lld\n", i, (b.std - b.boost) * 100. / b.std, b.std, b.boost);
  }
  return 1;
}
