// Copyright (C) 2010 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

// Including this will cause ambiguous errors in boost::move
#include <boost/unordered_map.hpp>

using namespace boost;

typedef upgrade_lock<shared_mutex> auto_upgrade_lock;
typedef upgrade_to_unique_lock<shared_mutex> auto_upgrade_unique_lock;

void testUpgrade(void)
{
  shared_mutex    mtx;
  auto_upgrade_lock lock(mtx);
  // Do some read-only stuff

  auto_upgrade_unique_lock writeLock(lock);
  // Do some write-only stuff with the upgraded lock
}

int main()
{
  testUpgrade();
  return 0;
}
