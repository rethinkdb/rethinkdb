//  libs/io/test/quote_manip_test.cpp  -----------------------------------------------  //

//  Copyright Beman Dawes 2010

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/io

//  ----------------------------------------------------------------------------------  //

#include <boost/io/detail/quoted_manip.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <iostream>
#include <sstream>

using boost::io::quoted;
using std::string;
using std::wstring;

int main()
{

  std::wstringstream wss;

  string r; // test results

  const string s0("foo");
  {
    std::stringstream ss;
    ss << quoted(s0);
    ss >> r;
    BOOST_TEST(r == "\"foo\"");
  }
  {
    std::stringstream ss;
    ss << quoted(s0);
    ss >> quoted(r);
    BOOST_TEST(r == "foo");
  }

  const string s0s("foo bar");
  {
    std::stringstream ss;
    ss << quoted(s0s);
    ss >> r;
    BOOST_TEST(r == "\"foo");
  }
  {
    std::stringstream ss;
    ss << quoted(s0s);
    ss >> quoted(r);
    BOOST_TEST(r == "foo bar");
  }

  const string s1("foo\\bar, \" *");
  {
    std::stringstream ss;
    ss << quoted(s1);
    ss >> r;
    BOOST_TEST(r == "\"foo\\\\bar,");
  }
  {
    std::stringstream ss;
    ss << quoted("foo\\bar, \" *");
    ss >> r;
    BOOST_TEST(r == "\"foo\\\\bar,");
  }
  {
    std::stringstream ss;
    ss << quoted(s1);
    ss >> quoted(r);
    BOOST_TEST(r == s1);
  }
  {
    std::stringstream ss;
    ss << quoted(s1.c_str());
    ss >> quoted(r);
    BOOST_TEST(r == s1);
  }

  string s2("'Jack & Jill'");                                 
  {
    std::stringstream ss;
    ss << quoted(s2, '&', '\'');
    ss >> quoted(r, '&', '\'');
    BOOST_TEST(r == s2);
  }

  wstring ws1(L"foo$bar, \" *");
  wstring wr; // test results
  {
    std::wstringstream wss;
    wss << quoted(ws1, L'$');
    wss >> quoted(wr,  L'$');
    BOOST_TEST(wr == ws1);
  }

  const string s3("const string");
  {
    std::stringstream ss;
    ss << quoted(s3);
    ss >> quoted(r);
    BOOST_TEST(r == s3);
  }
  {
    //  missing end delimiter test
    std::stringstream ss;
    ss << "\"abc";      // load ss with faulty quoting
    ss >> quoted(r);    // this loops if istream error/eof not detected
    BOOST_TEST(r == "abc");
  }
  {
    //  no initial delmiter test
    std::stringstream ss;
    ss << "abc";
    ss >> quoted(r);
    BOOST_TEST(r == "abc");
  }
  {
    //  no initial delmiter, space in ss
    std::stringstream ss;
    ss << "abc def";
    ss >> quoted(r);
    BOOST_TEST(r == "abc");
  }

  // these should fail to compile because the arguments are const:
  //   ss >> quoted(s1);
  //   ss >> quoted("foo");

  return boost::report_errors();
}
