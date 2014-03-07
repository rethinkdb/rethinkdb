// Copyright (C) 2011 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_USES_MOVE

#include <boost/config.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/container/list.hpp>
//#include <boost/interprocess/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr.hpp>
#include <iostream>
#include <boost/detail/lightweight_test.hpp>


int count = 0;
boost::mutex mutex;

namespace {


template <typename TC>
void join_all(TC & tc)
{
  for (typename TC::iterator it = tc.begin(); it != tc.end(); ++it)
  {
    (*it)->join();
  }
}


void increment_count()
{
  boost::unique_lock<boost::mutex> lock(mutex);
  std::cout << "count = " << ++count << std::endl;
}

template <class T>
struct default_delete
{
  typedef T*   pointer;

  BOOST_CONSTEXPR default_delete() BOOST_NOEXCEPT {} //= default;
  template <class U>
  default_delete(const default_delete<U>&) BOOST_NOEXCEPT
  {}
  void operator()(T* ptr) const
  {
    delete ptr;
  }
};

}
int main()
{
  {
    typedef boost::shared_ptr<boost::thread >  thread_ptr;
    //typedef boost::interprocess::shared_ptr<boost::thread, std::allocator<boost::thread>, default_delete<boost::thread> >  thread_ptr;
    typedef boost::container::list<thread_ptr > thread_ptr_list;
    thread_ptr_list threads;
    for (int i = 0; i < 10; ++i)
    {
      //threads.push_back(BOOST_THREAD_MAKE_RV_REF(thread_ptr(new boost::thread(&increment_count))));
      threads.push_back(thread_ptr(new boost::thread(&increment_count)));
    }
    BOOST_TEST(threads.size()==10);
    //join_all(threads);
    for (thread_ptr_list::iterator it = threads.begin(); it != threads.end(); ++it)
    {
      (*it)->join();
    }
  }
  count = 0;
  {
    typedef boost::shared_ptr<boost::thread >  thread_ptr;
    //typedef boost::interprocess::shared_ptr<boost::thread, std::allocator<boost::thread>, default_delete<boost::thread> >  thread_ptr;
    typedef boost::container::list<thread_ptr > thread_ptr_list;
    thread_ptr_list threads;
    for (int i = 0; i < 10; ++i)
    {
      //threads.push_back(BOOST_THREAD_MAKE_RV_REF(thread_ptr(new boost::thread(&increment_count))));
      threads.push_back(thread_ptr(new boost::thread(&increment_count)));
    }
    BOOST_TEST(threads.size()==10);
    thread_ptr sth(new boost::thread(&increment_count));
    threads.push_back(sth);
    BOOST_TEST(threads.size()==11);
    threads.remove(sth);
    BOOST_TEST(threads.size()==10);
    sth->join();
    //join_all(threads);
    for (thread_ptr_list::iterator it = threads.begin(); it != threads.end(); ++it)
    {
      (*it)->join();
    }
  }

  return boost::report_errors();
}
