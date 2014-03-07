/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_polymorphic2imp.cpp

// (C) Copyright 2009 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <boost/serialization/export.hpp>
#include <boost/archive/polymorphic_oarchive.hpp>
#include <boost/archive/polymorphic_iarchive.hpp>

#include "test_polymorphic2.hpp"

void A::serialize(
    boost::archive::polymorphic_oarchive &ar, 
    const unsigned int /*version*/
){
    ar & BOOST_SERIALIZATION_NVP(i);
}
void A::serialize(
    boost::archive::polymorphic_iarchive &ar, 
    const unsigned int /*version*/
){
    ar & BOOST_SERIALIZATION_NVP(i);
}
// note: only the most derived classes need be exported
// BOOST_CLASS_EXPORT(A)

void B::serialize(
    boost::archive::polymorphic_oarchive &ar, 
    const unsigned int /*version*/
){
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(A);
}
void B::serialize(
    boost::archive::polymorphic_iarchive &ar, 
    const unsigned int /*version*/
){
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(A);
}
BOOST_CLASS_EXPORT(B)
