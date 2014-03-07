/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include "../testfrmwk.hpp"
#include <iostream>


void test_month_decrement_iterator(const boost::gregorian::date *Answers, int array_len){
  using namespace boost::gregorian;
  typedef boost::date_time::month_functor<date> mfg;
    
  boost::date_time::date_itr<mfg, date> ditr(Answers[array_len-1]);
  int i = array_len-1;
  std::cout << "month iter decrement test..." << std::endl;
  try { 
    for (; ditr > Answers[0] - date_duration(1); --ditr) {
      check("month iterator: " + to_iso_string(*ditr), Answers[i] == *ditr);
      i--;
    }
    check("month iterator iteration count", i == -1);
  }
  catch(std::exception& e) 
  {
    check("month iterator: exception failure", false);      
    std::cout << e.what() << std::endl;
  }
}

void test_base_iterator(boost::gregorian::date end,
                        boost::gregorian::date_iterator& di,
                        std::string& data)
{
  using namespace boost::gregorian;
  for (; di < end; ++di) {
    data += to_iso_string(*di) + " ";
  }
}

int
main() 
{
  using namespace boost::gregorian;

  day_iterator di(date(2002,Jan,1));
  std::string data;
  test_base_iterator(date(2002,Jan,3),di,data);
  month_iterator di2(date(2002,Jan,3));
  test_base_iterator(date(2002,Mar,1),di2,data);
  std::string result("20020101 20020102 20020103 20020203 ");
  check("base iterator", data == result); 
  std::cout << data << std::endl;


  typedef boost::date_time::day_functor<date> dfg;
  
  {
    const date DayAnswers[] = {date(2000,Jan,20),date(2000,Jan,22),date(2000,Jan,24)};
    boost::date_time::date_itr<dfg,date> ditr(date(2000,Jan,20),2);
    int i=0;
    for (; ditr < date(2000,Jan,25); ++ditr) {
      //std::cout << *ditr << " ";
      check("day iterator -- 2 days", DayAnswers[i] == *ditr);
      i++;
    }
    check("day iterator -- 2 days", i == 3); // check the number of iterations
    // decrement
    i = 2;
    --ditr;
    for (; ditr > date(2000,Jan,19); --ditr) {
      //std::cout << *ditr << " ";
      check("day iterator decrement -- 2 days", DayAnswers[i] == *ditr);
      i--;
    }
    check("day iterator decrement -- 2 days", i == -1); // check the number of iterations
  }

  typedef boost::date_time::week_functor<date> wfg;
  {
    const date WeekAnswers[] = {date(2000,Jan,20),date(2000,Jan,27),date(2000,Feb,3)};
    boost::date_time::date_itr<wfg, date> ditr(date(2000,Jan,20));
    int i=0;
    for (; ditr < date(2000,Feb,6); ++ditr) {
      //std::cout << *ditr << " ";
      check("week iterator", WeekAnswers[i] == *ditr);
      i++;
    }
    check("week iterator", i == 3);
    // decrement
    i=2;
    --ditr;
    for (; ditr > date(2000,Jan,19); --ditr) {
      //std::cout << *ditr << " ";
      check("week iterator", WeekAnswers[i] == *ditr);
      i--;
    }
    check("week iterator", i == -1);
  }

  {
    const date WeekAnswers[] = {date(2000,Jan,20),date(2000,Feb,3)};
    boost::date_time::date_itr<wfg, date> ditr(date(2000,Jan,20),2);
    int i=0;
    for (; ditr < date(2000,Feb,6); ++ditr) {
      //std::cout << *ditr << " ";
      check("week iterator", WeekAnswers[i] == *ditr);
      i++;
    }
    check("week iterator", i == 2);
    // decrement
    i=1;
    --ditr;
    for (; ditr > date(2000,Jan,19); --ditr) {
      //std::cout << *ditr << " ";
      check("week iterator", WeekAnswers[i] == *ditr);
      i--;
    }
    check("week iterator", i == -1);
  }

  {
    const date WeekAnswers[] = {date(2000,Jan,20),date(2000,Feb,3), date(2000,Feb,17)};
    boost::date_time::date_itr<wfg, date> ditr(date(2000,Jan,20),2);
    int i=0;
    for (; ditr < date(2000,Feb,20); ++ditr) {
      //std::cout << *ditr << " ";
      check("week iterator -- 2 weeks", WeekAnswers[i] == *ditr);
      i++;
    }
    check("week iterator -- 2 weeks", i == 3);
    // decrement
    i=2;
    --ditr;
    for (; ditr > date(2000,Jan,19); --ditr) {
      //std::cout << *ditr << " ";
      check("week iterator -- 2 weeks", WeekAnswers[i] == *ditr);
      i--;
    }
    check("week iterator -- 2 weeks", i == -1);
  }

  typedef boost::date_time::month_functor<date> mfg;
  {
    const date MonthAnswers[] = {
      date(2000,Jan,1),date(2000,Feb,1),date(2000,Mar,1),date(2000,Apr,1),
      date(2000,May,1),date(2000,Jun,1),date(2000,Jul,1),date(2000,Aug,1),
      date(2000,Sep,1),date(2000,Oct,1),date(2000,Nov,1),date(2000,Dec,1),
      date(2001,Jan,1)
    };
    test_month_decrement_iterator(MonthAnswers, 13);
    
    boost::date_time::date_itr<mfg, date> ditr(date(2000,Jan,1));
    int i = 0;
    try { 
      for (; ditr < date(2001,Jan,2); ++ditr) {
        check("month iterator: " + to_iso_string(*ditr), MonthAnswers[i] == *ditr);
        i++;
      }
      check("month iterator iteration count", i == 13);
    }
    catch(std::exception& e) 
    {
      check("month iterator: exception failure", false);      
      std::cout << e.what() << std::endl;
    }
  }

  {
    const date MonthAnswers[] = {
      date(2000,Jan,31),date(2000,Feb,29),date(2000,Mar,31),date(2000,Apr,30),
      date(2000,May,31),date(2000,Jun,30),date(2000,Jul,31),date(2000,Aug,31),
      date(2000,Sep,30),date(2000,Oct,31),date(2000,Nov,30),date(2000,Dec,31),
      date(2001,Jan,31)
    };
    test_month_decrement_iterator(MonthAnswers, 13);
    
    boost::date_time::date_itr<mfg, date> ditr(date(2000,Jan,31));
    int i = 0;
    try { 
      for (; ditr < date(2001,Feb,1); ++ditr) {
        //      std::cout << *ditr << " ";
        check("last day of month iterator: " + to_iso_string(*ditr), 
              MonthAnswers[i] == *ditr);
        //check("last day of month iterator", MonthAnswers[i] == *ditr);
        i++;
      }
      check("last day of month iterator", i == 13);
    }
    catch(std::exception& e) 
    {
      check("last day of month iterator: exception failure", false);      
      std::cout << e.what() << std::endl;
    }
  }

  {
    const date MonthAnswers[] = {
      date(2000,Feb,29),date(2000,Mar,31),date(2000,Apr,30),
      date(2000,May,31),date(2000,Jun,30),date(2000,Jul,31),date(2000,Aug,31),
      date(2000,Sep,30),date(2000,Oct,31),date(2000,Nov,30),date(2000,Dec,31),
      date(2001,Jan,31),date(2001,Feb,28)
    };
    test_month_decrement_iterator(MonthAnswers, 13);
    
    boost::date_time::date_itr<mfg, date> ditr(date(2000,Feb,29));
    int i = 0;
    try { 
      for (; ditr < date(2001,Mar,1); ++ditr) {
        //      std::cout << *ditr << " ";
        check("last day of month iterator2: " + to_iso_string(*ditr), 
              MonthAnswers[i] == *ditr);
        //check("last day of month iterator", MonthAnswers[i] == *ditr);
        i++;
      }
      check("last day of month iterator2", i == 13);
    }
    catch(std::exception& e) 
    {
      check("last day of month iterator: exception failure", false);      
      std::cout << e.what() << std::endl;
    }
  }

  { // running a span of 5 years to verify snap to end doesn't occur at next leap year
    const date MonthAnswers[] = {
      date(2000,Feb,28),date(2000,Mar,28),date(2000,Apr,28),date(2000,May,28),
      date(2000,Jun,28),date(2000,Jul,28),date(2000,Aug,28),date(2000,Sep,28),
      date(2000,Oct,28),date(2000,Nov,28),date(2000,Dec,28),date(2001,Jan,28),
      date(2001,Feb,28),date(2001,Mar,28),date(2001,Apr,28),date(2001,May,28),
      date(2001,Jun,28),date(2001,Jul,28),date(2001,Aug,28),date(2001,Sep,28),
      date(2001,Oct,28),date(2001,Nov,28),date(2001,Dec,28),date(2002,Jan,28),
      date(2002,Feb,28),date(2002,Mar,28),date(2002,Apr,28),date(2002,May,28),
      date(2002,Jun,28),date(2002,Jul,28),date(2002,Aug,28),date(2002,Sep,28),
      date(2002,Oct,28),date(2002,Nov,28),date(2002,Dec,28),date(2003,Jan,28),
      date(2003,Feb,28),date(2003,Mar,28),date(2003,Apr,28),date(2003,May,28),
      date(2003,Jun,28),date(2003,Jul,28),date(2003,Aug,28),date(2003,Sep,28),
      date(2003,Oct,28),date(2003,Nov,28),date(2003,Dec,28),date(2004,Jan,28),
      date(2004,Feb,28),date(2004,Mar,28),date(2004,Apr,28),date(2004,May,28),
      date(2004,Jun,28),date(2004,Jul,28),date(2004,Aug,28),date(2004,Sep,28),
      date(2004,Oct,28),date(2004,Nov,28),date(2004,Dec,28),date(2005,Jan,28),
    };
    test_month_decrement_iterator(MonthAnswers, 60);
    
    boost::date_time::date_itr<mfg, date> ditr(date(2000,Feb,28));
    int i = 0;
    try { 
      for (; ditr < date(2005,Feb,1); ++ditr) {
        //      std::cout << *ditr << " ";
        check("last day of month iterator3: " + to_iso_string(*ditr), 
              MonthAnswers[i] == *ditr);
        //check("last day of month iterator", MonthAnswers[i] == *ditr);
        i++;
      }
      check("last day of month iterator3", i == 60);
    }
    catch(std::exception& e) 
    {
      check("last day of month iterator: exception failure", false);      
      std::cout << e.what() << std::endl;
    }
  }

  typedef boost::date_time::year_functor<date> yfg;
  {
    const date YearAnswers[] = {
      date(2000,Jan,1),date(2001,Jan,1),date(2002,Jan,1),date(2003,Jan,1),
      date(2004,Jan,1),date(2005,Jan,1),date(2006,Jan,1),date(2007,Jan,1),
      date(2008,Jan,1),date(2009,Jan,1),date(2010,Jan,1)
    };
    
    boost::date_time::date_itr<yfg, date> d3(date(2000,Jan,1));
    int i = 0;
    for (; d3 < date(2010,Jan,2); ++d3) {
      //std::cout << *d3 << " ";
      check("year iterator: " + to_iso_string(*d3), YearAnswers[i] == *d3);
      i++;
    }
    std::cout << "Decrementing...." << std::endl;
    i = 10;
    --d3;
    for (; d3 > date(1999,Dec,31); --d3) {
      //std::cout << *d3 << " ";
      check("year iterator: " + to_iso_string(*d3), YearAnswers[i] == *d3);
      i--;
    }
 }
  { // WON'T snap top end of month
    const date YearAnswers[] = {
      date(2000,Feb,28),date(2001,Feb,28),date(2002,Feb,28),date(2003,Feb,28),
      date(2004,Feb,28),date(2005,Feb,28),date(2006,Feb,28),date(2007,Feb,28),
      date(2008,Feb,28),date(2009,Feb,28),date(2010,Feb,28)
    };
    
    boost::date_time::date_itr<yfg, date> d3(date(2000,Feb,28));
    int i = 0;
    for (; d3 < date(2010,Mar,1); ++d3) {
      //std::cout << *d3 << " ";
      check("year iterator: " + to_iso_string(*d3), YearAnswers[i] == *d3);
      i++;
    }
    std::cout << "Decrementing...." << std::endl;
    i = 10;
    --d3;
    for (; d3 > date(2000,Feb,27); --d3) {
      //std::cout << *d3 << " ";
      check("year iterator: " + to_iso_string(*d3), YearAnswers[i] == *d3);
      i--;
    }
 }
  {// WILL snap top end of month
    const date YearAnswers[] = {
      date(2000,Feb,29),date(2001,Feb,28),date(2002,Feb,28),date(2003,Feb,28),
      date(2004,Feb,29),date(2005,Feb,28),date(2006,Feb,28),date(2007,Feb,28),
      date(2008,Feb,29),date(2009,Feb,28),date(2010,Feb,28)
    };
    
    boost::date_time::date_itr<yfg, date> d3(date(2000,Feb,29));
    int i = 0;
    for (; d3 < date(2010,Mar,1); ++d3) {
      //std::cout << *d3 << " ";
      check("year iterator: " + to_iso_string(*d3), YearAnswers[i] == *d3);
      i++;
    }
    std::cout << "Decrementing...." << std::endl;
    i = 10;
    --d3;
    for (; d3 > date(2000,Feb,27); --d3) {
      //std::cout << *d3 << " ";
      check("year iterator: " + to_iso_string(*d3), YearAnswers[i] == *d3);
      i--;
    }
 }

  {
    std::cout << "Increment by 2 years...." << std::endl;
    const date YearAnswers[] = {
      date(2000,Jan,1),date(2002,Jan,1),
      date(2004,Jan,1),date(2006,Jan,1),
      date(2008,Jan,1),date(2010,Jan,1)
    };
    
    boost::date_time::date_itr<yfg, date> d3(date(2000,Jan,1),2);
    int i = 0;
    for (; d3 < date(2010,Jan,2); ++d3) {
      //std::cout << *d3 << " ";
      check("year iterator: " + to_iso_string(*d3), YearAnswers[i] == *d3);
      i++;
    }
    // decrement
    std::cout << "Decrementing...." << std::endl;
    i = 5;
    --d3;
    for (; d3 > date(1999,Dec,31); --d3) {
      //std::cout << *d3 << " ";
      check("year iterator: " + to_iso_string(*d3), YearAnswers[i] == *d3);
      i--;
    }
  }


  return printTestStats();
}

