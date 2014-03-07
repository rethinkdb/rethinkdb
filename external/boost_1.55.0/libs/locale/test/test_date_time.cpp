//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/locale/date_time.hpp>
#include <boost/locale/generator.hpp>
#include <boost/locale/formatting.hpp>
#include <boost/locale/localization_backend.hpp>
#include <iomanip>
#include "test_locale.hpp"

#ifdef BOOST_LOCALE_WITH_ICU
#include <unicode/uversion.h>
#define BOOST_ICU_VER (U_ICU_VERSION_MAJOR_NUM*100 + U_ICU_VERSION_MINOR_NUM)
#else
#define BOOST_ICU_VER 406
#endif

#ifdef BOOST_MSVC
#  pragma warning(disable : 4244) // loose data 
#endif

#define RESET() do { time_point = base_time_point; ss.str(""); } while(0)
#define TESTR(X) do { TEST(X); RESET(); } while(0)
//#define TESTEQSR(t,X) do { ss << (t); TESTR(ss.str() == X); } while(0)
#define TESTEQSR(t,X) do { ss << (t); if(ss.str()!=X) { std::cerr <<"[" << ss.str() <<"]!=[" <<X<<"]" << std::endl; } TESTR(ss.str() == X); } while(0)

int main()
{
    try {
        using namespace boost::locale;
        using namespace boost::locale::period;
        std::string def[] = { 
        #ifdef BOOST_LOCALE_WITH_ICU
            "icu" , 
        #endif
        #ifndef BOOST_LOCALE_NO_STD_BACKEND
            "std" ,
        #endif
        #ifndef BOOST_LOCALE_NO_POSIX_BACKEND
            "posix",
        #endif
        #ifndef BOOST_LOCALE_NO_WINAPI_BACKEND
            "winapi",
        #endif
        };
        for(int type = 0 ; type < int(sizeof(def)/sizeof(def[0])) ; type ++ ) {
            boost::locale::localization_backend_manager tmp_backend = boost::locale::localization_backend_manager::global();
            tmp_backend.select(def[type]);
            boost::locale::localization_backend_manager::global(tmp_backend);
            std::cout << "Testing for backend: " << def[type] << std::endl;
            std::string backend_name = def[type];
            {
 
                boost::locale::generator g;

                std::locale loc=g("en_US.UTF-8");

                std::locale::global(loc);
                
                std::string tz("GMT");
                time_zone::global(tz);
                calendar cal(loc,tz); 

                TEST(calendar() == cal);
                TEST(calendar(loc) == cal);
                TEST(calendar(tz) == cal);
                TEST(calendar(loc,"GMT+01:00") != cal);
                TEST(calendar(g("ru_RU.UTF-8")) != cal);

                TEST(cal.minimum(month())==0);
                TEST(cal.maximum(month())==11);
                TEST(cal.minimum(day())==1);
                TEST(cal.greatest_minimum(day())==1);
                TEST(cal.least_maximum(day())==28);
                TEST(cal.maximum(day())==31);

                TEST(calendar(g("ar_EG.UTF-8")).first_day_of_week() == 7);
                TEST(calendar(g("he_IL.UTF-8")).first_day_of_week() == 1);
                TEST(calendar(g("ru_RU.UTF-8")).first_day_of_week() == 2);

                std::ostringstream ss;
                ss.imbue(loc);
                ss<<boost::locale::as::time_zone(tz);
                
                date_time time_point;
                
                time_point=year(1970) + february() + day(5);

                ss << as::ftime("%Y-%m-%d")<< time_point;

                TEST(ss.str() == "1970-02-05");
                time_point = 3 * hour_12() + 1 * am_pm() + 33 * minute() + 13 * second(); 
                ss.str("");
                ss << as::ftime("%Y-%m-%d %H:%M:%S") << time_point;
                TEST( ss.str() == "1970-02-05 15:33:13"); ss.str("");

                time_t a_date = 3600*24*(31+4); // Feb 5th
                time_t a_time = 3600*15+60*33; // 15:33:05
                time_t a_timesec = 13;
                time_t a_datetime = a_date + a_time + a_timesec;

                date_time base_time_point=date_time(a_datetime);

                RESET();

                time_point += hour();
                TESTEQSR(time_point,"1970-02-05 16:33:13");

                TEST(time_point.minimum(day())==1);
                TEST(time_point.maximum(day())==28);

                time_point += year() * 2 + 1 *month();
                TESTEQSR(time_point,"1972-03-05 15:33:13");

                time_point -= minute();
                TESTEQSR( time_point, "1970-02-05 15:32:13");

                time_point <<= minute() * 30;
                TESTEQSR( time_point, "1970-02-05 15:03:13"); 

                time_point >>= minute(40);
                TESTEQSR( time_point, "1970-02-05 15:53:13");

                TEST((time_point + month()) / month() == 2);
                TEST(month(time_point + month(1)) == 2);
                TEST(time_point / month() == 1);
                TEST((time_point - month()) / month()== 0);
                TEST(time_point / month() == 1);
                TEST((time_point << month()) / month()== 2);
                TEST(time_point / month()== 1);
                TEST((time_point >> month()) / month()== 0);
                TEST(time_point / month()== 1);



                TEST( (time_point + 2 * hour() - time_point) / minute() == 120);
                TEST( (time_point + month()- time_point) / day() == 28);
                TEST( (time_point + 2* month()- (time_point+month())) / day() == 31);
                TEST( day(time_point + 2* month()- (time_point+month())) == 31);

                TESTEQSR( time_point + hour(), "1970-02-05 16:33:13"); 
                TESTEQSR( time_point - hour(2), "1970-02-05 13:33:13");
                TESTEQSR( time_point >> minute(), "1970-02-05 15:32:13"); 
                TESTEQSR( time_point << second(), "1970-02-05 15:33:14");

                TEST(time_point == time_point);
                TEST(!(time_point != time_point));
                TEST(time_point.get(hour()) == 15);
                TEST(time_point/hour() == 15);
                TEST(time_point+year() != time_point);
                TEST(time_point - minute() <= time_point);
                TEST(time_point <= time_point);
                TEST(time_point + minute() >= time_point);
                TEST(time_point >= time_point);

                TEST(time_point < time_point + second());
                TEST(!(time_point < time_point - second()));
                TEST(time_point > time_point - second());
                TEST(!(time_point > time_point + second()));

                TEST(time_point.get(day()) == 5);
                TEST(time_point.get(year()) == 1970);

                TEST(time_point.get(era()) == 1);
                TEST(time_point.get(year()) == 1970);
                TEST(time_point.get(extended_year()) == 1970);
                if(backend_name == "icu") {
                    time_point=extended_year(-3);
                    TEST(time_point.get(era()) == 0);
                    TEST(time_point.get(year()) == 4);
                }
                RESET();
                TEST(time_point.get(month()) == 1);
                TEST(time_point.get(day()) == 5);
                TEST(time_point.get(day_of_year()) == 36);
                TEST(time_point.get(day_of_week()) == 5);
                TEST(time_point.get(day_of_week_in_month())==1);
                time_point=date_time(a_datetime,calendar(g("ru_RU.UTF-8")));
                TEST(time_point.get(day_of_week_local()) == 4);
                time_point = year(2026) + january() + day(1);
                TEST(time_point.get(day_of_week()) == 5);
                TEST(time_point.get(week_of_year()) == 1);
                TEST(time_point.get(week_of_month()) == 1);
                time_point = day_of_week() * 1;
                TEST(time_point.get(day()) == 4);
                TEST(time_point.get(week_of_year()) == 1);
                TEST(time_point.get(week_of_month()) == 1);
                time_point += day() * 1;
                TEST(time_point.get(week_of_year()) == 2);
                TEST(time_point.get(week_of_month()) == 2);
                time_point = february() + day() * 2;
                

                TEST(time_point.get(week_of_year()) == 6);
                
                if(backend_name!="icu" || BOOST_ICU_VER<408) {
                    TEST(time_point.get(week_of_month()) == 1);
                }
                else {
                    // cldr changes
                    TEST(time_point.get(week_of_month()) == 2);
                }

                time_point = year(2010) + january() + day() * 3;
                
                if(backend_name!="icu" || BOOST_ICU_VER<408) {
                    TEST(time_point.get(week_of_year()) == 53);
                }
                else {
                    TEST(time_point.get(week_of_year()) == 1);
                }
                
                time_point = year()*2010 + january() + day() * 4;
                
                if(backend_name!="icu" || BOOST_ICU_VER<408) {
                    TEST(time_point.get(week_of_year()) == 1);
                }
                else {
                    TEST(time_point.get(week_of_year()) == 2);
                }
                time_point = year()*2010 + january() + day() * 10;
                
                if(backend_name!="icu" || BOOST_ICU_VER<408) {
                    TEST(time_point.get(week_of_year()) == 1);
                }
                else {
                    TEST(time_point.get(week_of_year()) == 2);
                }

                time_point = year()*2010 + january() + day() * 11;
                if(backend_name!="icu" || BOOST_ICU_VER<408) {
                    TEST(time_point.get(week_of_year()) == 2);
                }
                else {
                    TEST(time_point.get(week_of_year()) == 3);
                }
                RESET();
                TEST(time_point.get(hour()) == 15);
                TEST(date_time(a_datetime,calendar("GMT+01:00")).get(hour()) ==16);
                TEST(time_point.get(hour_12()) == 3);
                TEST(time_point.get(am_pm()) == 1);
                TEST(time_point.get(minute()) == 33);
                TEST(time_point.get(second()) == 13);
                TEST(date_time(year()* 1984 + february() + day()).get(week_of_year())==5);
                TEST(time_point.get(week_of_month()) == 1);
                RESET();
                
                // Make sure we don't get year() < 1970 so the test would
                // work on windows where mktime supports positive time_t
                // only
                time_point = year() * 2010;

                TEST((time_point + year() *1 - hour() * 1 - time_point) / year() == 0);
                TEST((time_point + year() *1 - time_point) / year() == 1);
                TEST((time_point + year() *1 + hour() * 1 - time_point) / year() == 1);
                TEST((time_point - year() *1 + hour() * 1 - time_point) / year() == 0);
                TEST((time_point - year() *1 - time_point) / year() == -1);
                TEST((time_point - year() *1 - hour() * 1 - time_point) / year() == -1);

                RESET();

                time_point.time(24*3600 * 2);

                time_point = year() * 2011;
                time_point = march();
                time_point = day() * 29;

                date_time tmp_save = time_point;

                time_point = year() * 2011;
                time_point = february();
                time_point = day() * 5;

                TEST(time_point.get(year()) == 2011);
                TEST(time_point.get(month()) == 2); // march
                TEST(time_point.get(day()) == 5);
                
                time_point = tmp_save;

                time_point = year() * 2011 + february() + day() * 5;
                TEST(time_point.get(year()) == 2011);
                TEST(time_point.get(month()) == 1); // february
                TEST(time_point.get(day()) == 5);

            } // test
        }   // for loop

    }
    catch(std::exception const &e) {
        std::cerr << "Failed " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    FINALIZE();

}
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
// boostinspect:noascii 
