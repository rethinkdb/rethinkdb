/* Copyright (c) 2002,2003, 2007 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland 
 */

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/local_time_adjustor.hpp"
#include "boost/date_time/local_timezone_defs.hpp"
#include "../testfrmwk.hpp"

int
main() 
{
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  date dst_start(2002,Apr, 7);
  date dst_end_day(2002,Oct, 27);  

  typedef boost::date_time::utc_adjustment<time_duration,-5> us_eastern_offset_adj;
  //Type that embeds rules for UTC-5 plus DST offset
  typedef boost::date_time::static_local_time_adjustor<ptime, 
                                                  us_dst, 
                                                  us_eastern_offset_adj> us_eastern;
  
  //test some times clearly not in DST
  date d3(2002,Feb,1);
  ptime t10(d3, hours(4));
  ptime t10_check(d3, hours(9)); //utc is 5 hours ahead
  time_duration td = us_eastern::local_to_utc_offset(t10);//dst flag is defaulted
  check("check local calculation",   td == hours(5));
  ptime t10_local = t10 + td;
  std::cout << to_simple_string(t10_local)
            << std::endl;
  check("check local calculation",   t10_local == t10_check);
  check("check utc is dst",          
        us_eastern::utc_to_local_offset(t10) == hours(-5));
  

  //something clearly IN dst
  date d4(2002,May,1);
  ptime t11(d4, hours(3));
  check("check local offset",us_eastern::local_to_utc_offset(t11) == hours(4));
  std::cout << to_simple_string(us_eastern::local_to_utc_offset(t11)) << std::endl;
  ptime t11_check(d4, hours(7));//now utc offset is only 4 hours
  ptime t11_local = t11 + us_eastern::local_to_utc_offset(t11);
  std::cout << to_simple_string(t11_local) << " "
            << to_simple_string(t11_check)
            << std::endl;
  check("check local calculation", t11_local == t11_check);
  //should get same offset with DST flag set
  check("check local offset-dst flag on",   
        us_eastern::local_to_utc_offset(t11, boost::date_time::is_dst) == hours(4));
  check("check local offset-dst flag override",   
        us_eastern::local_to_utc_offset(t11, boost::date_time::not_dst) == hours(5));
 

  //Check the start of dst boundary
  ptime l_not_dst(dst_start, time_duration(1,59,59)); //2002-Apr-07 01:59:59
  check("check local dst start boundary case",   
        us_eastern::local_to_utc_offset(l_not_dst) == hours(5));
  ptime u_not_dst(dst_start, time_duration(6,59,59));
  check("check utc dst start boundary case",   
        us_eastern::utc_to_local_offset(u_not_dst) == hours(-5));
  ptime l_in_dst(dst_start, hours(3)); //2002-Apr-07 03:00:00 1st sec of dst
  check("check local dst start boundary case",   
        us_eastern::local_to_utc_offset(l_in_dst, boost::date_time::is_dst) == hours(4));
  ptime u_in_dst(dst_start, hours(7));
  check("check utc dst start boundary case",   
        us_eastern::utc_to_local_offset(u_in_dst) == hours(-4));


  //Check the end of dst boundary
  ptime dst_end(dst_end_day, time_duration(1,59,59)); //2002-Oct-27 01:00:00 DST
  check("check local dst end boundary case - still dst",   
        us_eastern::local_to_utc_offset(dst_end, boost::date_time::is_dst) == hours(4));
  check("check local dst end boundary case - still dst",   
        us_eastern::local_to_utc_offset(dst_end, boost::date_time::not_dst) == hours(5));
  ptime u_dst_end1(dst_end_day, time_duration(5,59,59));
  check("check utc dst end boundary case",   
        us_eastern::utc_to_local_offset(u_dst_end1) == hours(-4));
  ptime u_dst_end2(dst_end_day, time_duration(6,0,0));
  check("check utc dst end boundary case",   
        us_eastern::utc_to_local_offset(u_dst_end2) == hours(-5));
  ptime u_dst_end3(dst_end_day, time_duration(6,59,59));
  check("check utc dst end boundary case",   
        us_eastern::utc_to_local_offset(u_dst_end3) == hours(-5));
  ptime u_dst_end4(dst_end_day, time_duration(7,0,0));
  check("check utc dst end boundary case",   
        us_eastern::utc_to_local_offset(u_dst_end4) == hours(-5));
  

  //Now try a local adjustments without dst
  typedef boost::date_time::utc_adjustment<time_duration,-7> us_az_offset_adj;
  typedef boost::date_time::null_dst_rules<date, time_duration> us_az_dst_adj;
  //Type that embeds rules for UTC-7 with no dst
  typedef boost::date_time::static_local_time_adjustor<ptime, 
                                                  us_az_dst_adj, 
                                                  us_az_offset_adj> us_az;

  check("check local offset - no dst",   
        us_az::local_to_utc_offset(t10) == hours(7));
  check("check local offset - no dst",   
        us_az::local_to_utc_offset(t11) == hours(7));
  check("check local offset - no dst",   
        us_az::utc_to_local_offset(t10) == hours(-7));
  check("check local offset - no dst",   
        us_az::utc_to_local_offset(t11) == hours(-7));


  //Arizona timezone is utc-7 with no dst
  typedef boost::date_time::local_adjustor<ptime, -7, no_dst> us_arizona;

  ptime t7(date(2002,May,31), hours(17)); 
  ptime t8 = us_arizona::local_to_utc(t7);
  ptime t9 = us_arizona::utc_to_local(t8);
  //converted to local then back ot utc
  check("check us_local_adjustor", t9 == t7);

  typedef boost::date_time::local_adjustor<ptime, -5, us_dst> us_eastern2;

  ptime t7a(date(2002,May,31), hours(17)); 
  ptime t7b = us_eastern2::local_to_utc(t7a);
  ptime t7c = us_eastern2::utc_to_local(t7b);
  //converted to local then back ot utc
  check("check us_local_adjustor", t7c == t7a);


  typedef boost::date_time::us_dst_trait<date> us_dst_traits;
  typedef boost::date_time::dst_calc_engine<date, time_duration, us_dst_traits>
    us_dst_calc2;

  typedef boost::date_time::local_adjustor<ptime, -5, us_dst_calc2> us_eastern3;
  {
    ptime t7a(date(2002,May,31), hours(17)); 
    ptime t7b = us_eastern3::local_to_utc(t7a);
    ptime t7c = us_eastern3::utc_to_local(t7b);
    //converted to local then back ot utc
    check("check us_local_adjustor3", t7c == t7a);
  }

  {
    ptime t7a(date(2007,Mar,11), hours(4)); 
    ptime t7b = us_eastern3::local_to_utc(t7a);
    ptime t7c = us_eastern3::utc_to_local(t7b);
    //converted to local then back ot utc
    check("check us_local_adjustor3 2007", t7c == t7a);
  }

  {
    ptime t7a(date(2007,Mar,11), hours(3)); 
    ptime t7b = us_eastern3::local_to_utc(t7a);
    ptime t7c = us_eastern3::utc_to_local(t7b);
    //converted to local then back ot utc
    check("check us_local_adjustor3 2007 a", t7c == t7a);
  }

  //still experimental
  typedef boost::date_time::dynamic_local_time_adjustor<ptime, us_dst> lta;
//  lta adjustor(hours(-7));
  check("dst start", lta::local_dst_start_day(2002) == dst_start);
  check("dst end",   lta::local_dst_end_day(2002) == dst_end_day);
  check("dst boundary",   lta::is_dst_boundary_day(dst_start));
  check("dst boundary",   lta::is_dst_boundary_day(dst_end_day));
//   check("check non-dst offset",   adjustor.utc_offset(false)==hours(-7));
//   check("check dst offset",   adjustor.utc_offset(true)==hours(-6));
  

  check("dst start", lta::local_dst_start_day(2007) == date(2007,Mar,11));
  check("dst end",   lta::local_dst_end_day(2007) == date(2007,Nov,4));

  return printTestStats();

}

