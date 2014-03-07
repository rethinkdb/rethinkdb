/* The following shows the creation of a facet for the output of 
 * dates in German (please forgive me for any errors in my German --
 * I'm not a native speaker).
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include <iostream>
#include <algorithm>

/* Define a series of char arrays for short and long name strings 
 * to be associated with German date output (US names will be 
 * retrieved from the locale). */
const char* const de_short_month_names[] = 
{
  "Jan", "Feb", "Mar", "Apr", "Mai", "Jun",
  "Jul", "Aug", "Sep", "Okt", "Nov", "Dez", "NAM" 
};
const char* const de_long_month_names[] =
{
  "Januar", "Februar", "Marz", "April", "Mai",
  "Juni", "Juli", "August", "September", "Oktober",
  "November", "Dezember", "NichtDerMonat"
};
const char* const de_long_weekday_names[] = 
{
  "Sonntag", "Montag", "Dienstag", "Mittwoch",
  "Donnerstag", "Freitag", "Samstag"
};
const char* const de_short_weekday_names[] =
{
  "Son", "Mon", "Die","Mit", "Don", "Fre", "Sam"
};


int main() 
{
  using namespace boost::gregorian;
 
  // create some gregorian objects to output
  date d1(2002, Oct, 1);
  greg_month m = d1.month();
  greg_weekday wd = d1.day_of_week();
  
  // create a facet and a locale for German dates
  date_facet* german_facet = new date_facet();
  std::cout.imbue(std::locale(std::locale::classic(), german_facet));

  // create the German name collections
  date_facet::input_collection_type short_months, long_months, 
                                    short_weekdays, long_weekdays;
  std::copy(&de_short_month_names[0], &de_short_month_names[11],
            std::back_inserter(short_months));
  std::copy(&de_long_month_names[0], &de_long_month_names[11],
            std::back_inserter(long_months));
  std::copy(&de_short_weekday_names[0], &de_short_weekday_names[6],
            std::back_inserter(short_weekdays));
  std::copy(&de_long_weekday_names[0], &de_long_weekday_names[6],
            std::back_inserter(long_weekdays));

  // replace the default names with ours
  // NOTE: date_generators and special_values were not replaced as 
  // they are not used in this example
  german_facet->short_month_names(short_months);
  german_facet->long_month_names(long_months);
  german_facet->short_weekday_names(short_weekdays);
  german_facet->long_weekday_names(long_weekdays);
  
  // output the date in German using short month names
  german_facet->format("%d.%m.%Y");
  std::cout << d1 << std::endl; //01.10.2002
  
  german_facet->month_format("%B");
  std::cout << m << std::endl; //Oktober
  
  german_facet->weekday_format("%A");
  std::cout << wd << std::endl; //Dienstag


  // Output the same gregorian objects using US names
  date_facet* us_facet = new date_facet();
  std::cout.imbue(std::locale(std::locale::classic(), us_facet)); 

  us_facet->format("%m/%d/%Y");
  std::cout << d1 << std::endl; //  10/01/2002
  
  // English names, iso order (year-month-day), '-' separator
  us_facet->format("%Y-%b-%d");
  std::cout << d1 << std::endl; //  2002-Oct-01
  
  return 0;

}

/*  Copyright 2001-2005: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

