/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_const.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// compile only

#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/tracking.hpp>

using namespace boost::archive;

struct A {
    template<class Archive>
    void serialize(Archive & ar, unsigned int version) {
    }
};

void f1(text_oarchive & oa, A & a){
    oa << BOOST_SERIALIZATION_NVP(a);
}

