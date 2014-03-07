// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/sync_queue.hpp>

// class sync_queue<T>

//    sync_queue();

#define BOOST_THREAD_VERSION 4

#include <boost/thread/sync_queue.hpp>

#include <boost/detail/lightweight_test.hpp>

int main()
{

  {
    // default queue invariants
      boost::sync_queue<int> q;
      BOOST_TEST(q.empty());
      BOOST_TEST(! q.full());
      BOOST_TEST_EQ(q.size(), 0u);
      BOOST_TEST(! q.closed());
  }
  {
    // empty queue try_pull fails
      boost::sync_queue<int> q;
      int i;
      BOOST_TEST(! q.try_pull(i));
      BOOST_TEST(q.empty());
      BOOST_TEST(! q.full());
      BOOST_TEST_EQ(q.size(), 0u);
      BOOST_TEST(! q.closed());
  }
  {
    // empty queue try_pull fails
      boost::sync_queue<int> q;
      BOOST_TEST(! q.try_pull());
      BOOST_TEST(q.empty());
      BOOST_TEST(! q.full());
      BOOST_TEST_EQ(q.size(), 0u);
      BOOST_TEST(! q.closed());
  }
  {
    // empty queue push rvalue succeeds
      boost::sync_queue<int> q;
      q.push(1);
      BOOST_TEST(! q.empty());
      BOOST_TEST(! q.full());
      BOOST_TEST_EQ(q.size(), 1u);
      BOOST_TEST(! q.closed());
  }
  {
    // empty queue push rvalue succeeds
      boost::sync_queue<int> q;
      q.push(1);
      q.push(2);
      BOOST_TEST(! q.empty());
      BOOST_TEST(! q.full());
      BOOST_TEST_EQ(q.size(), 2u);
      BOOST_TEST(! q.closed());
  }
  {
    // empty queue push value succeeds
      boost::sync_queue<int> q;
      int i;
      q.push(i);
      BOOST_TEST(! q.empty());
      BOOST_TEST(! q.full());
      BOOST_TEST_EQ(q.size(), 1u);
      BOOST_TEST(! q.closed());
  }
  {
    // empty queue try_push rvalue succeeds
      boost::sync_queue<int> q;
      BOOST_TEST(q.try_push(1));
      BOOST_TEST(! q.empty());
      BOOST_TEST(! q.full());
      BOOST_TEST_EQ(q.size(), 1u);
      BOOST_TEST(! q.closed());
  }
  {
    // empty queue try_push value succeeds
      boost::sync_queue<int> q;
      int i;
      BOOST_TEST(q.try_push(i));
      BOOST_TEST(! q.empty());
      BOOST_TEST(! q.full());
      BOOST_TEST_EQ(q.size(), 1u);
      BOOST_TEST(! q.closed());
  }
  {
    // empty queue try_push rvalue succeeds
      boost::sync_queue<int> q;
      BOOST_TEST(q.try_push(boost::no_block, 1));
      BOOST_TEST(! q.empty());
      BOOST_TEST(! q.full());
      BOOST_TEST_EQ(q.size(), 1u);
      BOOST_TEST(! q.closed());
  }
  {
    // 1-element queue pull succeed
      boost::sync_queue<int> q;
      q.push(1);
      int i;
      q.pull(i);
      BOOST_TEST_EQ(i, 1);
      BOOST_TEST(q.empty());
      BOOST_TEST(! q.full());
      BOOST_TEST_EQ(q.size(), 0u);
      BOOST_TEST(! q.closed());
  }
  {
    // 1-element queue pull succeed
      boost::sync_queue<int> q;
      q.push(1);
      int i = q.pull();
      BOOST_TEST_EQ(i, 1);
      BOOST_TEST(q.empty());
      BOOST_TEST(! q.full());
      BOOST_TEST_EQ(q.size(), 0u);
      BOOST_TEST(! q.closed());
  }
  {
    // 1-element queue try_pull succeed
      boost::sync_queue<int> q;
      q.push(1);
      int i;
      BOOST_TEST(q.try_pull(i));
      BOOST_TEST_EQ(i, 1);
      BOOST_TEST(q.empty());
      BOOST_TEST(! q.full());
      BOOST_TEST_EQ(q.size(), 0u);
      BOOST_TEST(! q.closed());
  }
  {
    // 1-element queue try_pull succeed
      boost::sync_queue<int> q;
      q.push(1);
      int i;
      BOOST_TEST(q.try_pull(boost::no_block, i));
      BOOST_TEST_EQ(i, 1);
      BOOST_TEST(q.empty());
      BOOST_TEST(! q.full());
      BOOST_TEST_EQ(q.size(), 0u);
      BOOST_TEST(! q.closed());
  }
  {
    // 1-element queue try_pull succeed
      boost::sync_queue<int> q;
      q.push(1);
      boost::shared_ptr<int> i = q.try_pull();
      BOOST_TEST_EQ(*i, 1);
      BOOST_TEST(q.empty());
      BOOST_TEST(! q.full());
      BOOST_TEST_EQ(q.size(), 0u);
      BOOST_TEST(! q.closed());
  }

  {
    // closed invariants
      boost::sync_queue<int> q;
      q.close();
      BOOST_TEST(q.empty());
      BOOST_TEST(! q.full());
      BOOST_TEST_EQ(q.size(), 0u);
      BOOST_TEST(q.closed());
  }
  {
    // closed queue push fails
      boost::sync_queue<int> q;
      q.close();
      try {
        q.push(1);
        BOOST_TEST(false);
      } catch (...) {
        BOOST_TEST(q.empty());
        BOOST_TEST(! q.full());
        BOOST_TEST_EQ(q.size(), 0u);
        BOOST_TEST(q.closed());
      }
  }
  {
    // 1-element closed queue pull succeed
      boost::sync_queue<int> q;
      q.push(1);
      q.close();
      int i;
      q.pull(i);
      BOOST_TEST_EQ(i, 1);
      BOOST_TEST(q.empty());
      BOOST_TEST(! q.full());
      BOOST_TEST_EQ(q.size(), 0u);
      BOOST_TEST(q.closed());
  }

  return boost::report_errors();
}

