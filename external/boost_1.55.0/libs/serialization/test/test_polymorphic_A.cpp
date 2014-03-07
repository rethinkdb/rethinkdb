/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_polymorphic_A.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "test_polymorphic_A.hpp"
#include <boost/serialization/nvp.hpp>

#include "A.hpp"
#include "A.ipp"

data::data() :
    a(new A)
{}
data::~data(){
    delete a;
}

bool data::operator==(const data & rhs) const {
    return * (a) == *(rhs.a);
}

#if 0 // this method fails with msvc 6.0 and borland
// now we can define the serialization for class A
template<class Archive>
void data::serialize(Archive & ar, const unsigned int /* file_version */){
    ar & BOOST_SERIALIZATION_NVP(a);
}

// without the explicit instantiations below, the program will
// fail to link for lack of instantiantiation of the above function
// note: the following failed to fix link errors for vc 7.0 !
#include <boost/archive/polymorphic_oarchive.hpp>

template void data::serialize<boost::archive::polymorphic_oarchive>(
    boost::archive::polymorphic_oarchive & ar, 
    const unsigned int file_version
);

#include <boost/archive/polymorphic_iarchive.hpp>

template void data::serialize<boost::archive::polymorphic_iarchive>(
    boost::archive::polymorphic_iarchive & ar, 
    const unsigned int file_version
);
#endif

// so use this
void data::serialize(boost::archive::polymorphic_oarchive & ar, const unsigned int /* file_version */){
    ar & BOOST_SERIALIZATION_NVP(a);
}

void data::serialize(boost::archive::polymorphic_iarchive & ar, const unsigned int /* file_version */){
    ar & BOOST_SERIALIZATION_NVP(a);
}
