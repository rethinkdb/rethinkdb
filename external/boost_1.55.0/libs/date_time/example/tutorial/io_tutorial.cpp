#include <iostream>
#include <boost/date_time/local_time/local_time.hpp>

int main(){
  using namespace boost::gregorian;
  using namespace boost::posix_time;
  using namespace boost::local_time;
  using namespace std;

  /****** basic use ******/
  date d(2004, Feb, 29);
  time_duration td(12,34,56,789);
  stringstream ss;
  ss << d << ' ' << td;
  ptime pt(not_a_date_time);
  cout << pt << endl; // "not-a-date-time"
  ss >> pt;
  cout << pt << endl; // "2004-Feb-29 12:34:56.000789"
  ss.str("");
  ss << pt << " EDT-05EDT,M4.1.0,M10.5.0";
  local_date_time ldt(not_a_date_time);
  ss >> ldt;
  cout << ldt << endl; // " 2004-Feb-29 12:34:56.000789 EDT"


  /****** format strings ******/
  local_time_facet* output_facet = new local_time_facet();
  local_time_input_facet* input_facet = new local_time_input_facet();
  ss.imbue(locale(locale::classic(), output_facet));
  ss.imbue(locale(ss.getloc(), input_facet));
  output_facet->format("%a %b %d, %H:%M %z");
  ss.str("");
  ss << ldt;
  cout << ss.str() << endl; // "Sun Feb 29, 12:34 EDT"

  output_facet->format(local_time_facet::iso_time_format_specifier);
  ss.str("");
  ss << ldt;
  cout << ss.str() << endl; // "20040229T123456.000789-0500"
  output_facet->format(local_time_facet::iso_time_format_extended_specifier);
  ss.str("");
  ss << ldt;
  cout << ss.str() << endl; // "2004-02-29 12:34:56.000789-05:00"
 
  // extra words in format
  string my_format("The extended ordinal time %Y-%jT%H:%M can also be represented as %A %B %d, %Y");
  output_facet->format(my_format.c_str());
  input_facet->format(my_format.c_str());
  ss.str("");
  ss << ldt;
  cout << ss.str() << endl;

  // matching extra words in input 
  ss.str("The extended ordinal time 2005-128T12:15 can also be represented as Sunday May 08, 2005");
  ss >> ldt;
  cout << ldt << endl; // cout is using default format "2005-May-08 12:15:00 UTC"

  /****** content strings ******/
  // set up the collections of custom strings.
  // only the full names are altered for the sake of brevity
  string month_names[12] = { "january", "february", "march", 
                             "april", "may", "june", 
                             "july", "august", "september", 
                             "october", "november", "december" };
  vector<string> long_months(&month_names[0], &month_names[12]);
  string day_names[7] = { "sunday", "monday", "tuesday", "wednesday", 
                          "thursday", "friday", "saturday" };
  vector<string> long_days(&day_names[0], &day_names[7]);
  
  //  create date_facet and date_input_facet using all defaults
  date_facet* date_output = new date_facet();
  date_input_facet* date_input = new date_input_facet();
  ss.imbue(locale(ss.getloc(), date_output)); 
  ss.imbue(locale(ss.getloc(), date_input));

  // replace names in the output facet
  date_output->long_month_names(long_months);
  date_output->long_weekday_names(long_days);
  
  // replace names in the input facet
  date_input->long_month_names(long_months);
  date_input->long_weekday_names(long_days);
  
  // customize month, weekday and date formats
  date_output->format("%Y-%B-%d");
  date_input->format("%Y-%B-%d");
  date_output->month_format("%B"); // full name
  date_input->month_format("%B"); // full name
  date_output->weekday_format("%A"); // full name
  date_input->weekday_format("%A"); // full name

  ss.str("");
  ss << greg_month(3);
  cout << ss.str() << endl; // "march"
  ss.str("");
  ss << greg_weekday(3);
  cout << ss.str() << endl; // "tuesday"
  ss.str("");
  ss << date(2005,Jul,4);
  cout << ss.str() << endl; // "2005-july-04"


  /****** special values ******/
  // reset the formats to defaults
  output_facet->format(local_time_facet::default_time_format);
  input_facet->format(local_time_input_facet::default_time_input_format);

  // create custom special_values parser and formatter objects
  // and add them to the facets
  string sv[5] = {"nadt","neg_inf", "pos_inf", "min_dt", "max_dt" };
  vector<string> sv_names(&sv[0], &sv[5]);
  special_values_parser sv_parser(sv_names.begin(), sv_names.end());
  special_values_formatter sv_formatter(sv_names.begin(), sv_names.end());
  output_facet->special_values_formatter(sv_formatter);
  input_facet->special_values_parser(sv_parser);

  ss.str("");
  ldt = local_date_time(not_a_date_time);
  ss << ldt;
  cout << ss.str() << endl; // "nadt"
  
  ss.str("min_dt");
  ss >> ldt;
  ss.str("");
  ss << ldt;
  cout << ss.str() << endl; // "1400-Jan-01 00:00:00 UTC"

  /****** date/time periods ******/
  // reset all formats to defaults
  date_output->format(date_facet::default_date_format);
  date_input->format(date_facet::default_date_format);
  date_output->month_format("%b");    // abbrev
  date_input->month_format("%b");     // abbrev
  date_output->weekday_format("%a");  // abbrev
  date_input->weekday_format("%a");   // abbrev

  // create our date_period
  date_period dp(date(2005,Mar,1), days(31)); // month of march

  // custom period formatter and parser
  period_formatter per_formatter(period_formatter::AS_OPEN_RANGE, 
                                 " to ", "from ", " exclusive", " inclusive" );
  period_parser per_parser(period_parser::AS_OPEN_RANGE, 
                           " to ", "from ", " exclusive" , " inclusive" );
  
  // default output
  ss.str("");
  ss << dp;
  cout << ss.str() << endl; // "[2005-Mar-01/2005-Mar-31]"
 
  // add out custom parser and formatter to  the facets
  date_output->period_formatter(per_formatter);
  date_input->period_parser(per_parser);
  
  // custom output
  ss.str("");
  ss << dp;
  cout << ss.str() << endl; // "from 2005-Feb-01 to 2005-Apr-01 exclusive"
  
 
  /****** date generators ******/
  // custom date_generator phrases
  string dg_phrases[9] = { "1st", "2nd", "3rd", "4th", "5th", 
                           "final", "prior to", "following", "in" };
  vector<string> phrases(&dg_phrases[0], &dg_phrases[9]);

  // create our date_generator
  first_day_of_the_week_before d_gen(Monday);

  // default output
  ss.str("");
  ss << d_gen;
  cout << ss.str() << endl; // "Mon before"
 
  // add our custom strings to the date facets
  date_output->date_gen_phrase_strings(phrases);
  date_input->date_gen_element_strings(phrases);
  
  // custom output
  ss.str("");
  ss << d_gen;
  cout << ss.str() << endl; // "Mon prior to"
  
  return 0;
}


/*  Copyright 2005: CrystalClear Software, Inc
 *  http://www.crystalclearsoftware.com
 *
 *  Subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

