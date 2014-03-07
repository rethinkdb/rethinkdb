//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/locale.hpp>
#include <iostream>
#include <iomanip>
#include <ctime>

int main()
{
    using namespace boost::locale;

    generator gen;
    std::locale::global(gen(""));
    std::cout.imbue(std::locale());
    // Setup environment

    boost::locale::date_time now;

    date_time start=now;

    // Set the first day of the first month of this year
    start.set(period::month(),now.minimum(period::month()));
    start.set(period::day(),start.minimum(period::day()));

    int current_year = period::year(now);


    // Display current year
    std::cout << format("{1,ftime='%Y'}") % now << std::endl;

    //
    // Run forward untill current year is the date
    //
    for(now=start; period::year(now) == current_year;) {

        // Print heading of month
        if(calendar().is_gregorian()) 
            std::cout << format("{1,ftime='%B'}") % now <<std::endl;
        else
            std::cout << format("{1,ftime='%B'} ({1,ftime='%Y-%m-%d',locale=en} - {2,locale=en,ftime='%Y-%m-%d'})")
                % now 
                % date_time(now,now.maximum(period::day())*period::day()) << std::endl;

        int first = calendar().first_day_of_week();

        // Print weeks days
        for(int i=0;i<7;i++) {
            date_time tmp(now,period::day_of_week() * (first + i));
            std::cout << format("{1,w=8,ftime='%a'} ") % tmp;
        }
        std::cout << std::endl;

        int current_month = now / period::month();
        int skip = now / period::day_of_week_local() - 1;
        for(int i=0;i<skip*9;i++)
            std::cout << ' ';
        for(;now / period::month() == current_month ;now += period::day()) {
            std::cout << format("{1,w=8,ftime='%e'} ") % now;     
            if(now / period::day_of_week_local() == 7)
                std::cout << std::endl;
        }
        std::cout << std::endl;
    }

}
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
