// Copyright (C) 2010 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// bm.cpp

//  g++ test.cpp -lboost_thread-mt && ./a.out

// the ration of XXX and YYY determines
// if this works or deadlocks
int XXX = 20;
int YYY = 10;

#include <boost/thread/thread_only.hpp>
#include <boost/thread/shared_mutex.hpp>

//#include <unistd.h>
#include <iostream>
#include <boost/detail/lightweight_test.hpp>

using namespace std;

//void sleepmillis(useconds_t miliis)
void sleepmillis(int miliis)
{
  //usleep(miliis * 1000);
  boost::this_thread::sleep(boost::posix_time::milliseconds(miliis));
}

void worker1(boost::shared_mutex * lk, int * x)
{
  (*x)++; // 1
  cout << "lock b try " << *x << endl;
  while (1)
  {
    if (lk->timed_lock(boost::posix_time::milliseconds(XXX))) break;
    sleepmillis(YYY);
  }
  cout << "lock b got " << *x << endl;
  (*x)++; // 2
  lk->unlock();
}

void worker2(boost::shared_mutex * lk, int * x)
{
  cout << "lock c try" << endl;
  lk->lock_shared();
  (*x)++;
  cout << "lock c got" << endl;
  lk->unlock_shared();
  cout << "lock c unlocked" << endl;
  (*x)++;
}

int main()
{

  // create
  boost::shared_mutex* lk = new boost::shared_mutex();

  // read lock
  cout << "lock a" << endl;
  lk->lock_shared();

  int x1 = 0;
  boost::thread t1(boost::bind(worker1, lk, &x1));
  while (!x1)
    ;
  BOOST_TEST(x1 == 1);
  sleepmillis(500);
  BOOST_TEST(x1 == 1);

  int x2 = 0;
  boost::thread t2(boost::bind(worker2, lk, &x2));
  t2.join();
  BOOST_TEST(x2 == 2);

  lk->unlock_shared();
  cout << "unlock a" << endl;

  for (int i = 0; i < 2000; i++)
  {
    if (x1 == 2) break;
    sleepmillis(10);
  }

  BOOST_TEST(x1 == 2);
  t1.join();
  delete lk;

  return 0;
}
