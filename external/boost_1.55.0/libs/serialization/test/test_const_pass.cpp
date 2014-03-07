/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_const.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// compile only

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/nvp.hpp>

struct A {
    template<class Archive>
    void serialize(Archive & ar, unsigned int version) {
    }
};

// should compile w/o problem
void f1(boost::archive::text_oarchive & oa, const A & a){
    oa & a;
    oa & BOOST_SERIALIZATION_NVP(a);
    oa << a;
    oa << BOOST_SERIALIZATION_NVP(a);
}
void f2(boost::archive::text_oarchive & oa, const A * const & a){
    oa & a;
    oa & BOOST_SERIALIZATION_NVP(a);
    oa << a;
    oa << BOOST_SERIALIZATION_NVP(a);
}
void f3(boost::archive::text_iarchive & ia, A & a){
    ia & a;
    ia & BOOST_SERIALIZATION_NVP(a);
    ia >> a;
    ia >> BOOST_SERIALIZATION_NVP(a);
}
void f4(boost::archive::text_iarchive & ia, A * & a){
    ia & a;
    ia & BOOST_SERIALIZATION_NVP(a);
    ia >> a;
    ia >> BOOST_SERIALIZATION_NVP(a);
}
#if 0
void f5(boost::archive::text_oarchive & oa, const A * & a){
    oa & a;
    oa & BOOST_SERIALIZATION_NVP(a);
    oa << a;
    oa << BOOST_SERIALIZATION_NVP(a);
}
#endif

