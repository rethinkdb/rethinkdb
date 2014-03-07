/* Copyright (c) 2002-2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 */
 
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include "../testfrmwk.hpp"
#include <sstream>

using namespace boost;
using namespace gregorian;

template<class archive_type, class temporal_type>
void save_to(archive_type& ar, const temporal_type& tt)
{
  ar << tt;
}

int main(){
  std::ostringstream oss;
  
  // NOTE: DATE_TIME_XML_SERIALIZE is only used in testing and is
  // defined in the testing Jamfile
#if defined(DATE_TIME_XML_SERIALIZE)
  std::cout << "Running xml archive tests" << std::endl;
  archive::xml_oarchive oa(oss);
#else
  std::cout << "Running text archive tests" << std::endl;
  archive::text_oarchive oa(oss);
#endif
  
  date d(2002,Feb,12);
  date sv_d1(not_a_date_time);
  date sv_d2(pos_infin);
  date_duration dd(11);
  date_duration sv_dd(neg_infin);
  date_period dp(d,dd);
  greg_year gy(1959);
  greg_month gm(Feb);
  greg_day gd(14);
  greg_weekday gwd(Friday);
  partial_date pd(26,Jul);
  nth_kday_of_month nkd(nth_kday_of_month::second,Tuesday,Mar);
  first_kday_of_month fkd(Saturday,Apr);
  last_kday_of_month lkd(Saturday,Apr);
  first_kday_before fkdb(Thursday);
  first_kday_after fkda(Thursday);

  // load up the archive
  try{
#if defined(DATE_TIME_XML_SERIALIZE)
    save_to(oa, BOOST_SERIALIZATION_NVP(d));
    save_to(oa, BOOST_SERIALIZATION_NVP(sv_d1));
    save_to(oa, BOOST_SERIALIZATION_NVP(sv_d2));
    save_to(oa, BOOST_SERIALIZATION_NVP(dd));
    save_to(oa, BOOST_SERIALIZATION_NVP(sv_dd));
    save_to(oa, BOOST_SERIALIZATION_NVP(dp));
    save_to(oa, BOOST_SERIALIZATION_NVP(gy));
    save_to(oa, BOOST_SERIALIZATION_NVP(gm));
    save_to(oa, BOOST_SERIALIZATION_NVP(gd));
    save_to(oa, BOOST_SERIALIZATION_NVP(gwd));
    save_to(oa, BOOST_SERIALIZATION_NVP(pd));
    save_to(oa, BOOST_SERIALIZATION_NVP(nkd));
    save_to(oa, BOOST_SERIALIZATION_NVP(fkd));
    save_to(oa, BOOST_SERIALIZATION_NVP(lkd));
    save_to(oa, BOOST_SERIALIZATION_NVP(fkdb));
    save_to(oa, BOOST_SERIALIZATION_NVP(fkda));
#else
    save_to(oa, d);
    save_to(oa, sv_d1);
    save_to(oa, sv_d2);
    save_to(oa, dd);
    save_to(oa, sv_dd);
    save_to(oa, dp);
    save_to(oa, gy);
    save_to(oa, gm);
    save_to(oa, gd);
    save_to(oa, gwd);
    save_to(oa, pd);
    save_to(oa, nkd);
    save_to(oa, fkd);
    save_to(oa, lkd);
    save_to(oa, fkdb);
    save_to(oa, fkda);
#endif
  }catch(archive::archive_exception& ae){
    std::string s(ae.what());
    check("Error writing to archive: " + s + "\nWritten data: \"" + oss.str() + "\"", false);
    return printTestStats();
  }

  std::istringstream iss(oss.str());
#if defined(DATE_TIME_XML_SERIALIZE)
  archive::xml_iarchive ia(iss);
#else
  archive::text_iarchive ia(iss);
#endif
  
  // read from the archive
  date d2(not_a_date_time);
  date sv_d3(min_date_time);
  date sv_d4(min_date_time);
  date_duration dd2(not_a_date_time);
  date_duration sv_dd2(0);
  date_period dp2(date(2000,Jan,1),date_duration(1));
  greg_year gy2(1960);
  greg_month gm2(Jan);
  greg_day gd2(1);
  greg_weekday gwd2(Monday);
  partial_date pd2(1);
  nth_kday_of_month nkd2(nth_kday_of_month::first,Monday,Jan);
  first_kday_of_month fkd2(Monday,Jan);
  last_kday_of_month lkd2(Monday,Jan);
  first_kday_before fkdb2(Monday);
  first_kday_after fkda2(Monday);

  try{
#if defined(DATE_TIME_XML_SERIALIZE)
    ia >> BOOST_SERIALIZATION_NVP(d2);
    ia >> BOOST_SERIALIZATION_NVP(sv_d3);
    ia >> BOOST_SERIALIZATION_NVP(sv_d4);
    ia >> BOOST_SERIALIZATION_NVP(dd2);
    ia >> BOOST_SERIALIZATION_NVP(sv_dd2);
    ia >> BOOST_SERIALIZATION_NVP(dp2);
    ia >> BOOST_SERIALIZATION_NVP(gy2);
    ia >> BOOST_SERIALIZATION_NVP(gm2);
    ia >> BOOST_SERIALIZATION_NVP(gd2);
    ia >> BOOST_SERIALIZATION_NVP(gwd2);
    ia >> BOOST_SERIALIZATION_NVP(pd2);
    ia >> BOOST_SERIALIZATION_NVP(nkd2);
    ia >> BOOST_SERIALIZATION_NVP(fkd2);
    ia >> BOOST_SERIALIZATION_NVP(lkd2);
    ia >> BOOST_SERIALIZATION_NVP(fkdb2);
    ia >> BOOST_SERIALIZATION_NVP(fkda2);
#else
    ia >> d2;
    ia >> sv_d3;
    ia >> sv_d4;
    ia >> dd2;
    ia >> sv_dd2;
    ia >> dp2;
    ia >> gy2;
    ia >> gm2;
    ia >> gd2;
    ia >> gwd2;
    ia >> pd2;
    ia >> nkd2;
    ia >> fkd2;
    ia >> lkd2;
    ia >> fkdb2;
    ia >> fkda2;
#endif
  }catch(archive::archive_exception& ae){
    std::string s(ae.what());
    check("Error reading from archive: " + s + "\nWritten data: \"" + oss.str() + "\"", false);
    return printTestStats();
  }
  
  check("date", d == d2);
  check("special_value date (nadt)", sv_d1 == sv_d3);
  check("special_value date (pos_infin)", sv_d2 == sv_d4);
  check("date_duration", dd == dd2);
  check("special_value date_duration (neg_infin)", sv_dd == sv_dd2);
  check("date_period", dp == dp2);
  check("greg_year", gy == gy2);
  check("greg_month", gm == gm2);
  check("greg_day", gd == gd2);
  check("greg_weekday", gwd == gwd2);
  check("date_generator: partial_date", pd == pd2);
  check("date_generator: nth_kday_of_month", nkd.get_date(2002) == nkd2.get_date(2002)); // no operator== for nth_kday_of_week - yet
  check("date_generator: first_kday_of_month", fkd.get_date(2002) == fkd2.get_date(2002)); // no operator== for first_kday_of_week - yet
  check("date_generator: last_kday_of_month", lkd.get_date(2002) == lkd2.get_date(2002)); // no operator== for last_kday_of_week - yet
  check("date_generator: first_kday_before", fkdb.get_date(d) == fkdb2.get_date(d)); // no operator== for first_kday_before - yet
  check("date_generator: first_kday_after", fkda.get_date(d) == fkda2.get_date(d)); // no operator== for first_kday_after - yet

  return printTestStats();
}
