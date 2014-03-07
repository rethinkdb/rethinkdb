// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/sync_bounded_queue.hpp>

// class sync_queue<T>

//    push || pull;

#include <boost/config.hpp>
#if ! defined  BOOST_NO_CXX11_DECLTYPE
#define BOOST_RESULT_OF_USE_DECLTYPE
#endif

#define BOOST_THREAD_VERSION 4

#include <boost/thread/sync_bounded_queue.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/barrier.hpp>

#include <boost/detail/lightweight_test.hpp>

struct call_push
{
  boost::sync_bounded_queue<int> &q_;
  boost::barrier& go_;

  call_push(boost::sync_bounded_queue<int> &q, boost::barrier &go) :
    q_(q), go_(go)
  {
  }
  typedef void result_type;
  void operator()()
  {
    go_.count_down_and_wait();
    q_.push(42);

  }
};

struct call_push_2
{
  boost::sync_bounded_queue<int> &q_;
  boost::barrier& go_;
  boost::barrier& end_;

  call_push_2(boost::sync_bounded_queue<int> &q, boost::barrier &go, boost::barrier &end) :
    q_(q), go_(go), end_(end)
  {
  }
  typedef void result_type;
  void operator()()
  {
    go_.count_down_and_wait();
    q_.push(42);
    end_.count_down_and_wait();

  }
};

struct call_pull
{
  boost::sync_bounded_queue<int> &q_;
  boost::barrier& go_;

  call_pull(boost::sync_bounded_queue<int> &q, boost::barrier &go) :
    q_(q), go_(go)
  {
  }
  typedef int result_type;
  int operator()()
  {
    go_.count_down_and_wait();
    return q_.pull();
  }
};

void test_concurrent_push_and_pull_on_empty_queue()
{
  boost::sync_bounded_queue<int> q(4);

  boost::barrier go(2);

  boost::future<void> push_done;
  boost::future<int> pull_done;

  try
  {
    push_done=boost::async(boost::launch::async,
#if ! defined BOOST_NO_CXX11_LAMBDAS
        [&q,&go]()
        {
          go.wait();
          q.push(42);
        }
#else
        call_push(q,go)
#endif
    );
    pull_done=boost::async(boost::launch::async,
#if ! defined BOOST_NO_CXX11_LAMBDAS
        [&q,&go]() -> int
        {
          go.wait();
          return q.pull();
        }
#else
        call_pull(q,go)
#endif
    );

    push_done.get();
    BOOST_TEST_EQ(pull_done.get(), 42);
    BOOST_TEST(q.empty());
  }
  catch (...)
  {
    BOOST_TEST(false);
  }
}

void test_concurrent_push_on_empty_queue()
{
  boost::sync_bounded_queue<int> q(4);
  const unsigned int n = 3;
  boost::barrier go(n);
  boost::future<void> push_done[n];

  try
  {
    for (unsigned int i =0; i< n; ++i)
      push_done[i]=boost::async(boost::launch::async,
#if ! defined BOOST_NO_CXX11_LAMBDAS
        [&q,&go]()
        {
          go.wait();
          q.push(42);
        }
#else
        call_push(q,go)
#endif
    );

    for (unsigned int i = 0; i < n; ++i)
      push_done[i].get();

    BOOST_TEST(!q.empty());
    for (unsigned int i =0; i< n; ++i)
      BOOST_TEST_EQ(q.pull(), 42);
    BOOST_TEST(q.empty());

  }
  catch (...)
  {
    BOOST_TEST(false);
  }
}

void test_concurrent_push_on_full_queue()
{
  const unsigned int size = 2;
  boost::sync_bounded_queue<int> q(size);
  const unsigned int n = 2*size;
  boost::barrier go(n);
  boost::barrier end(size+1);
  boost::future<void> push_done[n];

  try
  {
    for (unsigned int i =0; i< n; ++i)
      push_done[i]=boost::async(boost::launch::async,
#if ! defined BOOST_NO_CXX11_LAMBDAS
        [&q,&go,&end]()
        {
          go.wait();
          q.push(42);
          end.wait();
        }
#else
        call_push_2(q,go,end)
#endif
      );

    end.wait();
    BOOST_TEST(!q.empty());
    BOOST_TEST(q.full());
    for (unsigned int i =0; i< size; ++i)
      BOOST_TEST_EQ(q.pull(), 42);
    end.wait();

    for (unsigned int i = 0; i < n; ++i)
      push_done[i].get();

    BOOST_TEST(!q.empty());
    for (unsigned int i =0; i< size; ++i)
      BOOST_TEST_EQ(q.pull(), 42);
    BOOST_TEST(q.empty());

  }
  catch (...)
  {
    BOOST_TEST(false);
  }
}
void test_concurrent_pull_on_queue()
{
  boost::sync_bounded_queue<int> q(4);
  const unsigned int n = 3;
  boost::barrier go(n);

  boost::future<int> pull_done[n];

  try
  {
    for (unsigned int i =0; i< n; ++i)
      q.push(42);

    for (unsigned int i =0; i< n; ++i)
      pull_done[i]=boost::async(boost::launch::async,
#if ! defined BOOST_NO_CXX11_LAMBDAS
        [&q,&go]() -> int
        {
          go.wait();
          return q.pull();
        }
#else
        call_pull(q,go)
#endif
    );

    for (unsigned int i = 0; i < n; ++i)
      BOOST_TEST_EQ(pull_done[i].get(), 42);
    BOOST_TEST(q.empty());
  }
  catch (...)
  {
    BOOST_TEST(false);
  }
}

int main()
{
  test_concurrent_push_and_pull_on_empty_queue();
  test_concurrent_push_on_empty_queue();
  test_concurrent_push_on_full_queue();
  test_concurrent_pull_on_queue();

  return boost::report_errors();
}

