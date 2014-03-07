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
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include "../testfrmwk.hpp"
#include <sstream>

using namespace boost;
using namespace posix_time;
using namespace gregorian;

template<class archive_type, class temporal_type>
void save_to(archive_type& ar, const temporal_type& tt)
{
  ar << tt;
}

int main(){
  // originals
  date d(2002, Feb, 14);
#if defined(BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG)
  time_duration td(12,13,52,123456789);
#else
  time_duration td(12,13,52,123456);
#endif
  ptime pt(d, td);
  time_period tp(pt, ptime(date(2002, Oct, 31), hours(19)));
  ptime sv_pt1(not_a_date_time);
  ptime sv_pt2(pos_infin);
  time_duration sv_td(neg_infin);

  // for loading in from archive
  date d2(not_a_date_time);
  time_duration td2(1,0,0);
  ptime pt2(d2, td2);
  time_period tp2(pt2, hours(1));
  ptime sv_pt3(min_date_time);
  ptime sv_pt4(min_date_time);
  time_duration sv_td2(0,0,0);
  
  std::ostringstream oss;

  // NOTE: DATE_TIME_XML_SERIALIZE is only used in testing and is
  // defined in the testing Jamfile
#if defined(DATE_TIME_XML_SERIALIZE)
  std::cout << "Running xml archive tests" << std::endl;
  archive::xml_oarchive oa(oss);
#else
  std::cout << "Running text archive tests" << std::endl;
  archive::text_oarchive oa(oss);
#endif // DATE_TIME_XML_SERIALIZE

  try{
#if defined(DATE_TIME_XML_SERIALIZE)
    save_to(oa, BOOST_SERIALIZATION_NVP(pt));
    save_to(oa, BOOST_SERIALIZATION_NVP(sv_pt1));
    save_to(oa, BOOST_SERIALIZATION_NVP(sv_pt2));
    save_to(oa, BOOST_SERIALIZATION_NVP(tp));
    save_to(oa, BOOST_SERIALIZATION_NVP(td));
    save_to(oa, BOOST_SERIALIZATION_NVP(sv_td));
#else
    save_to(oa, pt);
    save_to(oa, sv_pt1);
    save_to(oa, sv_pt2);
    save_to(oa, tp);
    save_to(oa, td);
    save_to(oa, sv_td);
#endif // DATE_TIME_XML_SERIALIZE
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
#endif // DATE_TIME_XML_SERIALIZE

  try{
#if defined(DATE_TIME_XML_SERIALIZE)
    ia >> BOOST_SERIALIZATION_NVP(pt2);
    ia >> BOOST_SERIALIZATION_NVP(sv_pt3);
    ia >> BOOST_SERIALIZATION_NVP(sv_pt4);
    ia >> BOOST_SERIALIZATION_NVP(tp2);
    ia >> BOOST_SERIALIZATION_NVP(td2);
    ia >> BOOST_SERIALIZATION_NVP(sv_td2);
#else
    ia >> pt2;
    ia >> sv_pt3;
    ia >> sv_pt4;
    ia >> tp2;
    ia >> td2;
    ia >> sv_td2;
#endif // DATE_TIME_XML_SERIALIZE
  }catch(archive::archive_exception& ae){
    std::string s(ae.what());
    check("Error readng from archive: " + s + "\nWritten data: \"" + oss.str() + "\"", false);
    return printTestStats();
  }

  check("ptime", pt == pt2);
  check("special_values ptime (nadt)", sv_pt1 == sv_pt3);
  check("special_values ptime (pos_infin)", sv_pt2 == sv_pt4);
  check("time_period", tp == tp2);
  check("time_duration", td == td2);
  check("special_values time_duration (neg_infin)", sv_td == sv_td2);

  return printTestStats();
}
