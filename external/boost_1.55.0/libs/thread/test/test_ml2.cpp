// Copyright (C) 2010 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 4

#include <cassert>
#include <vector>
#include <future>
#include <functional>
#include <boost/thread/future.hpp>


int TRUC = 42;
int main()
{
   std::vector< std::function<void()> > work_queue;

  auto do_some_work = [&]()-> boost::future<int*>
  {
    auto promise = std::make_shared<boost::promise<int*>>();
#if 0
    work_queue.push_back( [=]
    {
      promise->set_value( &TRUC );
    });
#else
    auto inner = [=]()
    {
      promise->set_value( &TRUC );
    };
    work_queue.push_back(inner);

#endif

    return promise->get_future();

  };

  auto ft_value = do_some_work();

  while( !work_queue.empty() )
   {
#if 0
      auto work = work_queue.back();
#else
      std::function<void()> work;
      work = work_queue.back();
#endif
      work_queue.pop_back();
      work();
   }

  auto value = ft_value.get();
  assert( value == &TRUC );
  return 0;
}


