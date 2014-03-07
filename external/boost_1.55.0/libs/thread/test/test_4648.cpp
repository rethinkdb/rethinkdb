// Copyright (C) 2010 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/thread.hpp>
#include <boost/current_function.hpp>

class boostThreadLocksTest
{
public:
  boost::shared_mutex myMutex;
  //boost::upgrade_lock<boost::shared_mutex> myLock;
  static int firstFunction(boostThreadLocksTest *pBoostThreadLocksTest);
  static int secondFunction(boostThreadLocksTest *pBoostThreadLocksTest,
      boost::upgrade_lock<boost::shared_mutex>& upgr);
  boostThreadLocksTest()
    :myMutex()
     //, myLock(myMutex,boost::defer_lock_t())
  {}
};

int boostThreadLocksTest::firstFunction(boostThreadLocksTest *pBoostThreadLocksTest)
{
  std::cout<<"Entering "<<boost::this_thread::get_id()<<" "<<"firstFunction"<<std::endl;
  boost::upgrade_lock<boost::shared_mutex> myLock(pBoostThreadLocksTest->myMutex);
  pBoostThreadLocksTest->secondFunction(pBoostThreadLocksTest, myLock);
  std::cout<<"Returned From Call "<<boost::this_thread::get_id()<<" "<<"firstFunction"<<std::endl;
  std::cout<<"Returning from "<<boost::this_thread::get_id()<<" "<<"firstFunction"<<std::endl;
  return(0);
}
int boostThreadLocksTest::secondFunction(boostThreadLocksTest */*pBoostThreadLocksTest*/, boost::upgrade_lock<boost::shared_mutex>& upgr) {
  std::cout<<"Before Exclusive Locking "<<boost::this_thread::get_id()<<" "<<"secondFunction"<<std::endl;
  boost::upgrade_to_unique_lock<boost::shared_mutex> localUniqueLock(upgr);
  std::cout<<"After Exclusive Locking "<<boost::this_thread::get_id()<<" "<<"secondFunction"<<std::endl;
  return(0);
}
int main() {
    boostThreadLocksTest myObject;
    boost::thread_group myThreadGroup;
    myThreadGroup.create_thread(boost::bind(boostThreadLocksTest::firstFunction,&myObject));
    myThreadGroup.create_thread(boost::bind(boostThreadLocksTest::firstFunction,&myObject));
    myThreadGroup.create_thread(boost::bind(boostThreadLocksTest::firstFunction,&myObject));
    myThreadGroup.join_all();
    return 0;
}
