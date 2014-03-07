/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
//
// demo_log.cpp
//
// (C) Copyright 2009 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <cstdio>

#include "demo_gps.hpp"
#include "simple_log_archive.hpp"

int main(int argc, char *argv[])
{   
    // make the schedule
    bus_schedule schedule;

    // fill in the data
    // make a few stops
    bus_stop *bs0 = new bus_stop_corner(
        gps_position(34, 135, 52.560f),
        gps_position(134, 22, 78.30f),
        "24th Street", "10th Avenue"
    );
    bus_stop *bs1 = new bus_stop_corner(
        gps_position(35, 137, 23.456f),
        gps_position(133, 35, 54.12f),
        "State street", "Cathedral Vista Lane"
    );
    bus_stop *bs2 = new bus_stop_destination(
        gps_position(35, 136, 15.456f),
        gps_position(133, 32, 15.300f),
        "White House"
    );
    bus_stop *bs3 = new bus_stop_destination(
        gps_position(35, 134, 48.789f),
        gps_position(133, 32, 16.230f),
        "Lincoln Memorial"
    );

    // make a  routes
    bus_route route0;
    route0.append(bs0);
    route0.append(bs1);
    route0.append(bs2);

    // add trips to schedule
    schedule.append("bob", 6, 24, &route0);
    schedule.append("bob", 9, 57, &route0);
    schedule.append("alice", 11, 02, &route0);

    // make aother routes
    bus_route route1;
    route1.append(bs3);
    route1.append(bs2);
    route1.append(bs1);

    // add trips to schedule
    schedule.append("ted", 7, 17, &route1);
    schedule.append("ted", 9, 38, &route1);
    schedule.append("alice", 11, 47, &route1);

    // display the complete schedule
    simple_log_archive log(std::cout);
    log << schedule;

    delete bs0;
    delete bs1;
    delete bs2;
    delete bs3;
    return 0;
}
