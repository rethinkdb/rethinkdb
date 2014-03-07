/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_const.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// compile only

#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/nvp.hpp>

using namespace boost::archive;

struct A {
    template<class Archive>
    void serialize(Archive & ar, unsigned int version) {
    }
};

// comment out this test case as a reminder not to keep inserting it !!!
// we don't trap this as an error in order to permit things like
// X * xptr;
// save(..){
//     ar << xptr;
// }
// 
// for rational - consider the following example from demo.cpp
// 
// std::list<pair<trip_info, bus_route_info *> > schedule
// 
// its not obvious to me how this can be cast to:
// 
// std::list<pair<trip_info, const bus_route_info * const> > schedule

void f4(text_oarchive & oa, A * const & a){
    oa << BOOST_SERIALIZATION_NVP(a);
}
