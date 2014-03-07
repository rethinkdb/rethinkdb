/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// demo_pimpl_A.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "demo_pimpl_A.hpp"

// "hidden" definition of class B
struct B {
    int b;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* file_version */){
        ar & b;
    }
};

A::A() :
    pimpl(new B)
{}
A::~A(){
    delete pimpl;
}
// now we can define the serialization for class A
template<class Archive>
void A::serialize(Archive & ar, const unsigned int /* file_version */){
    ar & pimpl;
}
 
// without the explicit instantiations below, the program will
// fail to link for lack of instantiantiation of the above function
// note: the following failed to fix link errors for vc 7.0 !
template void A::serialize<boost::archive::text_iarchive>(
    boost::archive::text_iarchive & ar, 
    const unsigned int file_version
);
template void A::serialize<boost::archive::text_oarchive>(
    boost::archive::text_oarchive & ar, 
    const unsigned int file_version
);
