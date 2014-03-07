
#ifndef TEST_FRMWK_HPP___
#define TEST_FRMWK_HPP___

/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * $Date: 2010-06-09 10:36:07 -0700 (Wed, 09 Jun 2010) $
 */


#include <iostream>
#include <string>
#include <boost/config.hpp>

//! Really simple test framework for counting and printing
class TestStats
{
public:
  static TestStats& instance() {static TestStats ts; return ts;}
  void addPassingTest() {testcount_++; passcount_++;}
  void addFailingTest() {testcount_++;}
  unsigned int testcount() const {return testcount_;}
  unsigned int passcount() const {return passcount_;}
  void print(std::ostream& out = std::cout) const
  {
    out << testcount_ << " Tests Executed: " ;
    if (passcount() != testcount()) {
      out << (testcount() - passcount()) << " FAILURES";
    }
    else {
      out << "All Succeeded" << std::endl;
    }
    out << std::endl;
  }
private:
  TestStats() : testcount_(0), passcount_(0) {}
  unsigned int testcount_;
  unsigned int passcount_;
};


inline bool check(const std::string& testname, bool testcond) 
{
  TestStats& stat = TestStats::instance();
  if (testcond) {
    std::cout << "Pass :: " << testname << " " <<  std::endl;
    stat.addPassingTest();
    return true;
  }
  else {
    stat.addFailingTest();
    std::cout << "FAIL :: " << testname << " " <<  std::endl;
    return false;
  }
}

template< typename T, typename U >
inline bool check_equal(const std::string& testname, T const& left, U const& right)
{
  bool res = check(testname, left == right);
  if (!res)
  {
    std::cout << "        left = " << left << ", right = " << right << std::endl;
  }
  return res;
}

#ifndef BOOST_NO_STD_WSTRING
inline bool check_equal(const std::string& testname, std::wstring const& left, std::wstring const& right)
{
  bool res = check(testname, left == right);
  if (!res)
  {
    std::wcout << L"        left = " << left << L", right = " << right << std::endl;
  }
  return res;
}
#endif

inline int printTestStats()
{
  TestStats& stat = TestStats::instance();
  stat.print();
  return stat.testcount() - stat.passcount();
}

#endif
