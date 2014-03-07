#ifndef BOOST_SERIALIZATION_EXAMPLE_DEMO_GPS_HPP
#define BOOST_SERIALIZATION_EXAMPLE_DEMO_GPS_HPP

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
//
// demo_gps.hpp
//
// (C) Copyright 2002-4 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iomanip>
#include <iostream>
#include <fstream>

#include <boost/serialization/string.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/assume_abstract.hpp>

// This illustration models the bus system of a small city.
// This includes, multiple bus stops,  bus routes and schedules.
// There are different kinds of stops.  Bus stops in general will
// will appear on multiple routes.  A schedule will include
// muliple trips on the same route.

/////////////////////////////////////////////////////////////
// gps coordinate
//
// llustrates serialization for a simple type
//
class gps_position
{
    friend class boost::serialization::access;
    friend std::ostream & operator<<(std::ostream &os, const gps_position &gp);

    int degrees;
    int minutes;
    float seconds;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* file_version */){
        ar  & BOOST_SERIALIZATION_NVP(degrees)
            & BOOST_SERIALIZATION_NVP(minutes)
            & BOOST_SERIALIZATION_NVP(seconds);
    }

public:
    // every serializable class needs a constructor
    gps_position(){};
    gps_position(int _d, int _m, float _s) : 
        degrees(_d), minutes(_m), seconds(_s)
    {}
};

std::ostream & operator<<(std::ostream &os, const gps_position &gp)
{
    return os << ' ' << gp.degrees << (unsigned char)186 << gp.minutes << '\'' << gp.seconds << '"';
}

/////////////////////////////////////////////////////////////
// One bus stop
//
// illustrates serialization of serializable members
//

class bus_stop
{
    friend class boost::serialization::access;
    virtual std::string description() const = 0;
    gps_position latitude;
    gps_position longitude;

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_NVP(latitude);
        ar & BOOST_SERIALIZATION_NVP(longitude);
    }

protected:
    bus_stop(const gps_position & _lat, const gps_position & _long) :
        latitude(_lat), longitude(_long)
    {}
public:
    bus_stop(){}
    friend std::ostream & operator<<(std::ostream &os, const bus_stop &gp);
    virtual ~bus_stop(){}
};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(bus_stop)

std::ostream & operator<<(std::ostream &os, const bus_stop &bs)
{
    return os << bs.latitude << bs.longitude << ' ' << bs.description();
}

/////////////////////////////////////////////////////////////
// Several kinds of bus stops
//
// illustrates serialization of derived types
//
class bus_stop_corner : public bus_stop
{
    friend class boost::serialization::access;
    std::string street1;
    std::string street2;
    virtual std::string description() const
    {
        return street1 + " and " + street2;
    }
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        // save/load base class information
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(bus_stop);
        ar & BOOST_SERIALIZATION_NVP(street1);
        ar & BOOST_SERIALIZATION_NVP(street2);
    }
public:
    bus_stop_corner(){}
    bus_stop_corner(const gps_position & _lat, const gps_position & _long,
        const std::string & _s1, const std::string & _s2
    ) :
        bus_stop(_lat, _long), street1(_s1), street2(_s2)
    {
    }
};

class bus_stop_destination : public bus_stop
{
    friend class boost::serialization::access;
    std::string name;
    virtual std::string description() const
    {
        return name;
    }
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar  & BOOST_SERIALIZATION_BASE_OBJECT_NVP(bus_stop)
            & BOOST_SERIALIZATION_NVP(name);
    }
public:
    bus_stop_destination(){}
    bus_stop_destination(
        const gps_position & _lat, const gps_position & _long, const std::string & _name
    ) :
        bus_stop(_lat, _long), name(_name)
    {
    }
};

/////////////////////////////////////////////////////////////
// a bus route is a collection of bus stops
//
// illustrates serialization of STL collection templates.
//
// illustrates serialzation of polymorphic pointer (bus stop *);
//
// illustrates storage and recovery of shared pointers is correct
// and efficient.  That is objects pointed to by more than one
// pointer are stored only once.  In such cases only one such
// object is restored and pointers are restored to point to it
//
class bus_route
{
    friend class boost::serialization::access;
    friend std::ostream & operator<<(std::ostream &os, const bus_route &br);
    typedef bus_stop * bus_stop_pointer;
    std::list<bus_stop_pointer> stops;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        // in this program, these classes are never serialized directly but rather
        // through a pointer to the base class bus_stop. So we need a way to be
        // sure that the archive contains information about these derived classes.
        //ar.template register_type<bus_stop_corner>();
        ar.register_type(static_cast<bus_stop_corner *>(NULL));
        //ar.template register_type<bus_stop_destination>();
        ar.register_type(static_cast<bus_stop_destination *>(NULL));
        // serialization of stl collections is already defined
        // in the header
        ar & BOOST_SERIALIZATION_NVP(stops);
    }
public:
    bus_route(){}
    void append(bus_stop *_bs)
    {
        stops.insert(stops.end(), _bs);
    }
};
std::ostream & operator<<(std::ostream &os, const bus_route &br)
{
    std::list<bus_stop *>::const_iterator it;
    // note: we're displaying the pointer to permit verification
    // that duplicated pointers are properly restored.
    for(it = br.stops.begin(); it != br.stops.end(); it++){
        os << '\n' << std::hex << "0x" << *it << std::dec << ' ' << **it;
    }
    return os;
}

/////////////////////////////////////////////////////////////
// a bus schedule is a collection of routes each with a starting time
//
// Illustrates serialization of STL objects(pair) in a non-intrusive way.
// See definition of operator<< <pair<F, S> >(ar, pair)
// 
// illustrates nesting of serializable classes
//
// illustrates use of version number to automatically grandfather older
// versions of the same class.

class bus_schedule
{
    friend class boost::serialization::access;
    friend std::ostream & operator<<(std::ostream &os, const bus_schedule &bs);
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_NVP(schedule);
    }
    // note: this structure was made public. because the friend declarations
    // didn't seem to work as expected.
public:
    struct trip_info
    {
        template<class Archive>
        void serialize(Archive &ar, const unsigned int file_version)
        {
            // in versions 2 or later
            if(file_version >= 2)
                // read the drivers name
                ar & BOOST_SERIALIZATION_NVP(driver);
            // all versions have the follwing info
            ar  & BOOST_SERIALIZATION_NVP(hour)
                & BOOST_SERIALIZATION_NVP(minute);
        }

        // starting time
        int hour;
        int minute;
        // only after system shipped was the driver's name added to the class
        std::string driver;

        trip_info(){}
        trip_info(int _h, int _m, const std::string &_d) :
            hour(_h), minute(_m), driver(_d)
        {}
        ~trip_info(){
        }
    };
//  friend std::ostream & operator<<(std::ostream &os, const trip_info &ti);
private:
    std::list<std::pair<trip_info, bus_route *> > schedule;
public:
    void append(const std::string &_d, int _h, int _m, bus_route *_br)
    {
        schedule.insert(schedule.end(), std::make_pair(trip_info(_h, _m, _d), _br));
    }
    bus_schedule(){}
};

BOOST_CLASS_VERSION(bus_schedule::trip_info, 3)
BOOST_CLASS_VERSION(bus_schedule, 2)

std::ostream & operator<<(std::ostream &os, const bus_schedule::trip_info &ti)
{
    return os << '\n' << ti.hour << ':' << ti.minute << ' ' << ti.driver << ' ';
}
std::ostream & operator<<(std::ostream &os, const bus_schedule &bs)
{
    std::list<std::pair<bus_schedule::trip_info, bus_route *> >::const_iterator it;
    for(it = bs.schedule.begin(); it != bs.schedule.end(); it++){
        os << it->first << *(it->second);
    }
    return os;
}

#endif // BOOST_SERIALIZATION_EXAMPLE_DEMO_GPS_HPP
