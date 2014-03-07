// Copyright (C) 2010 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/thread.hpp>
#include <boost/thread/thread_only.hpp>
#include <boost/optional.hpp>
#include <boost/detail/lightweight_test.hpp>

using namespace boost;
using namespace boost::chrono;

struct dummy_class_tracks_deletions
{
    static unsigned deletions;

    dummy_class_tracks_deletions()
    {
      std::cout << __FILE__ << ":" << __LINE__ << boost::this_thread::get_id() << std::endl;
    }
    ~dummy_class_tracks_deletions()
    {
        ++deletions;
        std::cout << __FILE__ << ":" << __LINE__ << boost::this_thread::get_id() << std::endl;
    }

};
unsigned dummy_class_tracks_deletions::deletions=0;


optional<thread_specific_ptr<dummy_class_tracks_deletions> > optr;
//struct X
//{
//  thread_specific_ptr<int> f;
//} sptr;

void other_thread()
{

  std::cout << __FILE__ << ":" << __LINE__ << boost::this_thread::get_id() << std::endl;
  optr = none;
  std::cout << __FILE__ << ":" << __LINE__ << boost::this_thread::get_id() << std::endl;
  optr = in_place();
  std::cout << __FILE__ << ":" << __LINE__ << boost::this_thread::get_id() << std::endl;
  BOOST_TEST(optr->get() == 0);
  this_thread::sleep(posix_time::seconds(5));
  BOOST_TEST(optr->get() == 0);
  std::cout << __FILE__ << ":" << __LINE__ << boost::this_thread::get_id() << std::endl;

}

int main()
{

  std::cout << __FILE__ << ":" << __LINE__ << boost::this_thread::get_id() << std::endl;
  dummy_class_tracks_deletions * pi = new dummy_class_tracks_deletions;
  std::cout << __FILE__ << ":" << __LINE__ << boost::this_thread::get_id() << std::endl;
  optr = in_place();
  std::cout << __FILE__ << ":" << __LINE__ << boost::this_thread::get_id() << std::endl;
  optr->reset(pi);
  std::cout << __FILE__ << ":" << __LINE__ << boost::this_thread::get_id() << std::endl;
  BOOST_TEST(optr->get() == pi);
  thread t1(bind(&other_thread));
  this_thread::sleep(posix_time::seconds(5));
  std::cout << __FILE__ << ":" << __LINE__ << boost::this_thread::get_id() << std::endl;
  BOOST_TEST(optr->get() == pi);
  std::cout << __FILE__ << ":" << __LINE__ << boost::this_thread::get_id() << std::endl;
  t1.join();
  return boost::report_errors();

}
