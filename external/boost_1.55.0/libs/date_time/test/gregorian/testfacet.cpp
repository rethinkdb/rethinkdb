/* Copyright (c) 2002,2003,2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst 
 */

#include <sstream>
#include <iostream>
#include <fstream>

#include "boost/date_time/gregorian/greg_month.hpp"
#include "boost/date_time/gregorian/greg_facet.hpp"
#include "boost/date_time/date_format_simple.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "../testfrmwk.hpp"

#ifndef BOOST_DATE_TIME_NO_LOCALE

    const char* const de_short_month_names[]={"Jan","Feb","Mar","Apr","Mai","Jun","Jul","Aug","Sep","Okt","Nov","Dez", "NAM"};

    const char* const de_long_month_names[]={"Januar","Februar","Marz","April","Mai","Juni","Juli","August","September","Oktober","November","Dezember","NichtDerMonat"};
    const char* const de_special_value_names[]={"NichtDatumzeit","-unbegrenztheit", "+unbegrenztheit"};

const char* const de_short_weekday_names[]={"Son", "Mon", "Die","Mit", "Don", "Fre", "Sam"};

    const char* const de_long_weekday_names[]={"Sonntag", "Montag", "Dienstag","Mittwoch", "Donnerstag", "Freitag", "Samstag"};

#endif 

/** Not used for now
    const char* const es_short_month_names[]={"Ene","Feb","Mar","Abr","Pue","Jun","Jul","Ago","Sep","Oct","Nov","Dic", "NAM"};

    const char* const es_long_month_names[]={"Enero","Febrero","Marcha","Abril","Pueda","Junio","Julio","Agosto","Septiembre","Octubre","Noviembre","Diciembre","NoAMes"};
    const char* const es_special_value_names[]={"NoUnRatoDeLaFacha","-infinito", "+infinito"};
**/
int
main()
{
//   std::locale native("");
//   std::cout << "native: " << native.name() << std::endl;
//#ifndef BOOST_NO_STD_LOCALE
#ifndef BOOST_DATE_TIME_NO_LOCALE

  using namespace boost::gregorian;

  typedef greg_facet_config facet_config;
  typedef boost::date_time::all_date_names_put<facet_config> date_facet;
  typedef boost::date_time::date_names_put<facet_config> date_facet_base;
  typedef boost::date_time::ostream_month_formatter<date_facet_base> month_formatter;

  {
    // special_values tests
    std::stringstream ss;
    date_facet_base* f = new date_facet_base();
    std::locale loc(std::locale::classic(), f);
    ss.imbue(loc);
    date d(not_a_date_time);
    ss << d;
    check("Special value, stream out nadt" , ss.str() == std::string("not-a-date-time"));
    ss.str("");
    d = date(neg_infin);
    ss << d;
    check("Special value, stream out neg_infin" , ss.str() == std::string("-infinity"));
    ss.str("");
    d = date(pos_infin);
    ss << d;
    check("Special value, stream out pos_infin" , ss.str() == std::string("+infinity"));
  }

  date_facet gdnp(de_short_month_names, de_long_month_names, 
                  de_special_value_names, de_long_weekday_names,
                  de_long_weekday_names, 
                  '.', 
                  boost::date_time::ymd_order_dmy);

  std::stringstream ss;
  std::ostreambuf_iterator<char> coi(ss);
  gdnp.put_month_short(coi, Oct);
  check("check german short month: " + ss.str(), 
        ss.str() == std::string("Okt"));

  ss.str(""); //reset string stream 
  greg_month m(Oct);
  month_formatter::format_month(m, ss, gdnp);
  check("check german short month: " + ss.str(), 
        ss.str() == std::string("Okt"));
  ss.str(""); //reset string stream 
//   month_formatter::format_month(m, ss, gdnp);
//   check("check german long month: " + ss.str(), 
//         ss.str() == std::string("Oktober"));


  greg_year_month_day ymd(2002,Oct,1);
  typedef boost::date_time::ostream_ymd_formatter<greg_year_month_day, date_facet_base> ymd_formatter;
  ss.str(""); //reset string stream 
  ymd_formatter::ymd_put(ymd, ss, gdnp);
  check("check ymd: " + ss.str(), 
        ss.str() == std::string("01.Okt.2002"));


  typedef boost::date_time::ostream_date_formatter<date, date_facet_base> datef;

  std::stringstream os;
  date d1(2002, Oct, 1);
  datef::date_put(d1, os, gdnp);
  check("ostream low level check string:"+os.str(), 
        os.str() == std::string("01.Okt.2002"));

//   //Locale tests
  std::locale global;
  std::cout << "global: " << global.name() << std::endl;

  // put a facet into a locale
  //check for a facet p319
  check("no registered facet here",
        !std::has_facet<date_facet>(global));

  std::locale global2(global, 
                      new date_facet(de_short_month_names, 
                                     de_long_month_names,
                                     de_special_value_names,
                                     de_long_weekday_names,
                                     de_long_weekday_names));

  check("facet registered here",
        std::has_facet<boost::date_time::date_names_put<facet_config> >(global2));

  std::stringstream os2;
  os2.imbue(global2); 
  datef::date_put(d1, os2);
  check("check string imbued ostream: "+os2.str(), 
        os2.str() == std::string("2002-Okt-01"));

  date infin(pos_infin);
  os2.str(""); //clear stream
  datef::date_put(infin, os2);
  check("check string imbued ostream: "+os2.str(), 
        os2.str() == std::string("+unbegrenztheit"));

  os2.str(""); //clear stream
  os2 << infin;
  check("check string imbued ostream: "+os2.str(), 
        os2.str() == std::string("+unbegrenztheit"));


  date nadt(not_a_date_time);
  os2.str(""); //clear stream
  datef::date_put(nadt, os2);
  check("check string imbued ostream: "+os2.str(), 
        os2.str() == std::string("NichtDatumzeit"));
  

  std::stringstream os3;
  os3 << d1;
  check("check any old ostream: "+os3.str(), 
        os3.str() == std::string("2002-Oct-01"));

  std::ofstream f("test_facet_file.out");
  f << d1 << std::endl;
  
//   // date formatter that takes locale and gets facet from locale
  std::locale german_dates1(global, 
                            new date_facet(de_short_month_names, 
                                           de_long_month_names,
                                           de_special_value_names,
                                           de_short_weekday_names,
                                           de_long_weekday_names,
                                           '.',
                                           boost::date_time::ymd_order_dmy,
                                           boost::date_time::month_as_integer));

  os3.imbue(german_dates1); 
  os3.str("");
  os3 << d1;
  check("check date order: "+os3.str(), 
        os3.str() == std::string("01.10.2002"));

  std::locale german_dates2(global, 
                            new date_facet(de_short_month_names, 
                                           de_long_month_names,
                                           de_special_value_names,
                                           de_short_weekday_names,
                                           de_long_weekday_names,
                                           ' ',
                                           boost::date_time::ymd_order_iso,
                                           boost::date_time::month_as_short_string));

  os3.imbue(german_dates2); 
  os3.str("");
  os3 << d1;
  check("check date order: "+os3.str(), 
        os3.str() == std::string("2002 Okt 01"));

  std::locale german_dates3(global, 
                            new date_facet(de_short_month_names, 
                                           de_long_month_names,
                                           de_special_value_names,
                                           de_short_weekday_names,
                                           de_long_weekday_names,
                                           ' ',
                                           boost::date_time::ymd_order_us,
                                           boost::date_time::month_as_long_string));

  os3.imbue(german_dates3); 
  os3.str("");
  os3 << d1;
  check("check date order: "+os3.str(), 
        os3.str() == std::string("Oktober 01 2002"));

  date_period dp(d1, date_duration(3));
  os3.str("");
  os3 << dp;
  check("check date period: "+os3.str(), 
        os3.str() == std::string("[Oktober 01 2002/Oktober 03 2002]"));


  /*******************************************************************/
  /* Streaming operations for date durations                         */
  /*******************************************************************/

  date_duration dur(26);
  std::stringstream ss2;
  ss2 << dur;
  check("date_duration stream out", ss2.str() == std::string("26"));

  dur = date_duration(boost::date_time::pos_infin);
  ss2.str("");
  ss2 << dur;
  check("date_duration stream out", ss2.str() == std::string("+infinity"));

  /*******************************************************************/
  /* Streaming operations for date generator functions               */
  /*******************************************************************/

  partial_date pd(26, Jun);
  //std::stringstream ss2;
  ss2.str("");
  ss2 << pd;
  check("partial date stream out", ss2.str() == std::string("26 Jun"));

  ss2.str("");
  nth_kday_of_month nkm(nth_kday_of_month::second, Friday, Sep);
  ss2 << nkm;
  check("nth kday of month", ss2.str() == std::string("second Fri of Sep"));

  ss2.str("");
  first_kday_of_month fkm(Saturday, May);
  ss2 << fkm;
  check("first kday of month", ss2.str() == std::string("first Sat of May"));

  ss2.str("");
  last_kday_of_month lkm(Monday, Aug);
  ss2 << lkm;
  check("last kday of month", ss2.str() == std::string("last Mon of Aug"));

  ss2.str("");
  first_kday_after  fka(Thursday);//fkb.get_date(d)
  ss2 << fka;
  check("first kday after", ss2.str() == std::string("Thu after"));

  ss2.str("");
  first_kday_before fkb(Tuesday); // same ^
  ss2 << fkb;
  check("first kday after", ss2.str() == std::string("Tue before"));

  std::cout << pd << '\n'
            << nkm << '\n'
            << fkm << '\n'
            << lkm << '\n'
            << fka << '\n'
            << fkb << '\n'
            << std::endl;
  
  /*******************************************************************/
  /* Input Streaming for greg_month                                  */
  /*******************************************************************/
  {
    std::stringstream ss1("January");
    std::stringstream ss2("dec"); // misspelled
    std::stringstream german("Okt");
    german.imbue(global2);
    greg_month m(3);
    ss1 >> m;
    check("Stream in month", m == greg_month(Jan));
#ifndef BOOST_NO_STD_WSTRING
    std::wstringstream ws1(L"Dec");
    ws1 >> m;
    check("Wide Stream in month", m == greg_month(Dec));
#else
    check("Wide Stream in not supported by this compiler", false);
#endif // BOOST_NO_STD_WSTRING
    german >> m;
    check("Stream in German month", m == greg_month(Oct));
    try{
      ss2 >> m; // misspelled
      check("Bad month exception NOT thrown (misspelled name)", false);
    }catch(bad_month&){
      check("Bad month exception caught (misspelled name)", true);
    }catch(...){
      check("Bad month exception NOT caught (misspelled name)", false);
    }
  }
  /*******************************************************************/
  /* Input Streaming for greg_weekday                                */
  /*******************************************************************/
  {
    std::stringstream ss1("Sun");
    std::stringstream ss2("Wensday"); // misspelled
    std::stringstream german("Mittwoch"); // Wednesday
    german.imbue(global2);
    greg_weekday wd(Friday); //something not Sunday...
    ss1 >> wd;
    check("Stream in weekday", wd == greg_weekday(Sunday));
#ifndef BOOST_NO_STD_WSTRING
    std::wstringstream ws1(L"Saturday");
    ws1 >> wd;
    check("Wide Stream in weekday", wd == greg_weekday(Saturday));
#else
    check("Wide Stream in not supported by this compiler", false);
#endif // BOOST_NO_STD_WSTRING
    german >> wd;
    check("Stream in German weekday", wd == greg_weekday(Wednesday));
    try{
      ss2 >> wd;
      check("Bad weekday exception NOT thrown (misspelled name)", false);
    }catch(bad_weekday&){
      check("Bad weekday exception caught (misspelled name)", true);
    }catch(...){
      check("Bad weekday exception NOT caught (misspelled name)", false);
    }
  }

#else
  check("No tests executed - Locales not supported by this compiler", false);

#endif //BOOST_DATE_TIME_NO_LOCALE

  return printTestStats();

}

