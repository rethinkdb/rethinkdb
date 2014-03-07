// Copyright (C) 2012-2013 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/future.hpp>

// class future<R>

// template<typename F>
// auto then(F&& func) -> future<decltype(func(*this))>;

#define BOOST_THREAD_VERSION 4
#define BOOST_THREAD_USES_LOG
#define BOOST_THREAD_USES_LOG_THREAD_ID
#include <boost/thread/detail/log.hpp>

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>

#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION

int p1()
{
  BOOST_THREAD_LOG << "p1 < " << BOOST_THREAD_END_LOG;
  boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
  BOOST_THREAD_LOG << "p1 >"  << BOOST_THREAD_END_LOG;
  return 1;
}

int p2(boost::shared_future<int> f)
{
  BOOST_THREAD_LOG << "p2 <" << &f << BOOST_THREAD_END_LOG;
  BOOST_TEST(f.valid());
  int i = f.get();
  boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
  BOOST_THREAD_LOG << "p2 <" << &f << BOOST_THREAD_END_LOG;
  return 2 * i;
}

void p3(boost::shared_future<int> f)
{
  BOOST_THREAD_LOG << "p3 <" << &f << BOOST_THREAD_END_LOG;
  BOOST_TEST(f.valid());
  int i = f.get();
  boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
  BOOST_THREAD_LOG << "p3 <" << &f << " " << i << BOOST_THREAD_END_LOG;
  return ;
}

int main()
{
  BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
  {
    boost::shared_future<int> f1 = boost::async(boost::launch::async, &p1).share();
    BOOST_TEST(f1.valid());
    boost::future<int> f2 = f1.then(&p2);
    BOOST_TEST(f2.valid());
    try
    {
      BOOST_TEST(f2.get()==2);
    }
    catch (std::exception& ex)
    {
      BOOST_THREAD_LOG << "ERRORRRRR "<<ex.what() << "" << BOOST_THREAD_END_LOG;
      BOOST_TEST(false);
    }
    catch (...)
    {
      BOOST_THREAD_LOG << " ERRORRRRR exception thrown" << BOOST_THREAD_END_LOG;
      BOOST_TEST(false);
    }
  }
  BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
  {
    boost::shared_future<int> f1 = boost::async(boost::launch::async, &p1).share();
    BOOST_TEST(f1.valid());
    boost::future<void> f2 = f1.then(&p3);
    BOOST_TEST(f2.valid());
    try
    {
      f2.wait();
    }
    catch (std::exception& ex)
    {
      BOOST_THREAD_LOG << "ERRORRRRR "<<ex.what() << "" << BOOST_THREAD_END_LOG;
      BOOST_TEST(false);
    }
    catch (...)
    {
      BOOST_THREAD_LOG << " ERRORRRRR exception thrown" << BOOST_THREAD_END_LOG;
      BOOST_TEST(false);
    }
  }
  BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
  {
    boost::future<int> f2 = boost::async(p1).share().then(&p2);
    BOOST_TEST(f2.get()==2);
  }
  BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
  {
    boost::shared_future<int> f1 = boost::async(p1).share();
    boost::shared_future<int> f21 = f1.then(&p2).share();
    boost::future<int> f2= f21.then(&p2);
    BOOST_TEST(f2.get()==4);
  }
  BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
  {
    boost::shared_future<int> f1 = boost::async(p1).share();
    boost::future<int> f2= f1.then(&p2).share().then(&p2);
    BOOST_TEST(f2.get()==4);
  }
  BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
  {
    boost::future<int> f2 = boost::async(p1).share().then(&p2).share().then(&p2);
    BOOST_TEST(f2.get()==4);
  }

  return boost::report_errors();
}

#else

int main()
{
  return 0;
}
#endif
