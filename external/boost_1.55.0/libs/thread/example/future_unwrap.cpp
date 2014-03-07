// Copyright (C) 2012-2013 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 4
#define BOOST_THREAD_USES_LOG
#define BOOST_THREAD_USES_LOG_THREAD_ID

#include <boost/thread/detail/log.hpp>
#include <boost/thread/future.hpp>
#include <boost/assert.hpp>
#include <string>
#if defined BOOST_THREAD_PROVIDES_FUTURE_UNWRAP

int p1()
{
  BOOST_THREAD_LOG << "P1" << BOOST_THREAD_END_LOG;
  return 123;
}

boost::future<int> p2()
{
  BOOST_THREAD_LOG << "<P2" << BOOST_THREAD_END_LOG;
  boost::future<int> f1 = boost::async(boost::launch::async, &p1);
  BOOST_THREAD_LOG << "P2>" << BOOST_THREAD_END_LOG;
  return boost::move(f1);
}

int main()
{
  BOOST_THREAD_LOG << "<MAIN" << BOOST_THREAD_END_LOG;
  try
  {
    boost::future<boost::future<int> > outer_future = boost::async(boost::launch::async, &p2);
    boost::future<int> inner_future = outer_future.unwrap();
    int i = inner_future.get();
    BOOST_THREAD_LOG << "i= "<< i << "" << BOOST_THREAD_END_LOG;
  }
  catch (std::exception& ex)
  {
    BOOST_THREAD_LOG << "ERRORRRRR "<<ex.what() << "" << BOOST_THREAD_END_LOG;
    return 1;
  }
  catch (...)
  {
    BOOST_THREAD_LOG << " ERRORRRRR exception thrown" << BOOST_THREAD_END_LOG;
    return 2;
  }
  BOOST_THREAD_LOG << "MAIN>" << BOOST_THREAD_END_LOG;
  return 0;
}
#else

int main()
{
  return 0;
}
#endif
