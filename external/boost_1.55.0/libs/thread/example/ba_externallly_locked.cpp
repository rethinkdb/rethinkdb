// Copyright (C) 2012 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 4

#include <boost/thread/mutex.hpp>
#include <boost/thread/lockable_adapter.hpp>
#include <boost/thread/externally_locked.hpp>
#include <boost/thread/strict_lock.hpp>
#include <boost/thread/lock_types.hpp>
#include <iostream>

using namespace boost;

class BankAccount
{
  int balance_;
public:
  void Deposit(int amount)
  {
    balance_ += amount;
  }
  void Withdraw(int amount)
  {
    balance_ -= amount;
  }
  int GetBalance()
  {
    return balance_;
  }
};

//[AccountManager
class AccountManager: public basic_lockable_adapter<mutex>
{
public:
  typedef basic_lockable_adapter<mutex> lockable_base_type;
  AccountManager() :
    lockable_base_type(), checkingAcct_(*this), savingsAcct_(*this)
  {
  }
  inline void Checking2Savings(int amount);
  inline void AMoreComplicatedChecking2Savings(int amount);
private:
  /*<-*/
  bool some_condition()
  {
    return true;
  } /*->*/
  externally_locked<BankAccount, AccountManager > checkingAcct_;
  externally_locked<BankAccount, AccountManager > savingsAcct_;
};
//]

//[Checking2Savings
void AccountManager::Checking2Savings(int amount)
{
  strict_lock<AccountManager> guard(*this);
  checkingAcct_.get(guard).Withdraw(amount);
  savingsAcct_.get(guard).Deposit(amount);
}
//]

//#if DO_NOT_COMPILE
////[AMoreComplicatedChecking2Savings_DO_NOT_COMPILE
//void AccountManager::AMoreComplicatedChecking2Savings(int amount) {
//    unique_lock<AccountManager> guard(*this);
//    if (some_condition()) {
//        guard.lock();
//    }
//    checkingAcct_.get(guard).Withdraw(amount);
//    savingsAcct_.get(guard).Deposit(amount);
//    guard1.unlock();
//}
////]
//#elif DO_NOT_COMPILE_2
////[AMoreComplicatedChecking2Savings_DO_NOT_COMPILE2
//void AccountManager::AMoreComplicatedChecking2Savings(int amount) {
//    unique_lock<AccountManager> guard1(*this);
//    if (some_condition()) {
//        guard1.lock();
//    }
//    {
//        strict_lock<AccountManager> guard(guard1);
//        checkingAcct_.get(guard).Withdraw(amount);
//        savingsAcct_.get(guard).Deposit(amount);
//    }
//    guard1.unlock();
//}
////]
//#else
////[AMoreComplicatedChecking2Savings
void AccountManager::AMoreComplicatedChecking2Savings(int amount) {
    unique_lock<AccountManager> guard1(*this);
    if (some_condition()) {
        guard1.lock();
    }
    {
        nested_strict_lock<unique_lock<AccountManager> > guard(guard1);
        checkingAcct_.get(guard).Withdraw(amount);
        savingsAcct_.get(guard).Deposit(amount);
    }
    guard1.unlock();
}
////]
//#endif

int main()
{
  AccountManager mgr;
  mgr.Checking2Savings(100);
  return 0;
}

