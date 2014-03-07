// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef EVENTS_HPP
#define EVENTS_HPP

// events
struct play {};
struct end_pause {};
struct stop {};
struct pause {};
struct open_close {};

// A "complicated" event type that carries some data.
enum DiskTypeEnum
{
    DISK_CD=0,
    DISK_DVD=1
};
struct cd_detected
{
    cd_detected(std::string name, DiskTypeEnum diskType)
        : name(name),
        disc_type(diskType)
    {}

    std::string name;
    DiskTypeEnum disc_type;
};


#endif
