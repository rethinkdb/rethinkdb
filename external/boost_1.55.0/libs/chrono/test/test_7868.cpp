//  Copyright 2013 Vicente J. Botet Escriba
//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt
//  See http://www.boost.org/libs/chrono for documentation.

#define BOOST_CHRONO_VERSION 2
//#define BOOST_CHRONO_PROVIDES_DATE_IO_FOR_SYSTEM_CLOCK_TIME_POINT 1

#include <sstream>
#include <iostream>
#include <boost/chrono/chrono_io.hpp>
#include <boost/chrono/floor.hpp>
#include <boost/chrono/round.hpp>
#include <boost/chrono/ceil.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <cstdio>

int main()
{
  using namespace boost;
  using namespace boost::chrono;
  boost::chrono::system_clock::time_point atnow= boost::chrono::system_clock::now();
  {
      std::stringstream strm;
      std::stringstream strm2;
      // does not change anything: strm<<time_fmt(boost::chrono::timezone::utc);
      // does not change anything: strm2<<time_fmt(boost::chrono::timezone::utc);
      boost::chrono::system_clock::time_point atnow2;
      strm<<atnow<<std::endl;
      time_t t = boost::chrono::system_clock::to_time_t(atnow);
      std::cout << "A:" << std::endl;
      puts(ctime(&t));
      std::cout << "A:" << std::endl;
      std::cout << "A:" << strm.str()<< std::endl;
      std::cout << "A:" << atnow.time_since_epoch().count() << std::endl;
      strm>>atnow2;
      strm2<<atnow2<<std::endl;
      std::cout << "B:" << strm2.str()<< std::endl;
      std::cout << "B:" << atnow2.time_since_epoch().count()<< std::endl;
      BOOST_TEST_EQ(atnow.time_since_epoch().count(), atnow2.time_since_epoch().count());

      // 1 sec wrong:
      std::cout << "diff:" << boost::chrono::duration_cast<microseconds>(atnow2 - atnow).count() <<std::endl;
      BOOST_TEST_EQ(boost::chrono::duration_cast<microseconds>(atnow2 - atnow).count(), 0);
      std::stringstream formatted;
      formatted << time_fmt(boost::chrono::timezone::utc, "%Y-%m-%d %H:%M:%S");
      formatted << "actual:"<< atnow <<std::endl;
      formatted << "parsed:"<< atnow2 <<std::endl;
      std::cout << formatted.str();
      std::stringstream formatted1;
      formatted1 << time_fmt(boost::chrono::timezone::utc, "%Y-%m-%d %H:%M:%S");
      formatted1 << atnow ;
      std::stringstream formatted2;
      formatted2 << time_fmt(boost::chrono::timezone::utc, "%Y-%m-%d %H:%M:%S");
      formatted2 << atnow2 ;
      BOOST_TEST_EQ(formatted1.str(), formatted2.str());


  }

  {
    std::cout << "FORMATTED" << std::endl;
      std::stringstream strm;
      std::stringstream strm2;
      boost::chrono::system_clock::time_point atnow2;
      // the final second mark is always parsed as 01
      strm<<time_fmt(boost::chrono::timezone::utc, "%Y-%m-%d %H:%M:%S");
      strm2<<time_fmt(boost::chrono::timezone::utc, "%Y-%m-%d %H:%M:%S");
      strm<<atnow<<std::endl;
      std::cout << "actual:" << strm.str()<< std::endl;
      strm>>atnow2;
      strm2<<atnow2<<std::endl;
      // the final second mark is always parsed as 01
      std::cout << "parsed:" << strm2.str()<< std::endl;
      //BOOST_TEST_EQ(atnow, atnow2); // fails because the pattern doesn't contains nanoseconds
      //BOOST_TEST_EQ(atnow.time_since_epoch().count(), atnow2.time_since_epoch().count()); // fails because the pattern doesn't contains nanoseconds
      BOOST_TEST_EQ(boost::chrono::duration_cast<seconds>(atnow2 - atnow).count(), 0);

  }
  return boost::report_errors();
}
