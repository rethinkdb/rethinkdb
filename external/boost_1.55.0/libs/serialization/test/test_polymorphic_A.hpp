/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_polymorphic_A.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// class whose declaration is hidden by a pointer
// #include <boost/serialization/scoped_ptr.hpp>

#include <boost/archive/polymorphic_oarchive.hpp>
#include <boost/archive/polymorphic_iarchive.hpp>

class A;

struct data {
    // class a contains a pointer to a "hidden" declaration
    // borland scoped_ptr doesn't work !!!
    // boost::scoped_ptr<A> a;
    A * a;
//    template<class Archive>
//    void serialize(Archive & ar, const unsigned int file_version);
    void serialize(boost::archive::polymorphic_oarchive & ar, const unsigned int file_version);
    void serialize(boost::archive::polymorphic_iarchive & ar, const unsigned int file_version);
    data();
    ~data();
    bool operator==(const data & rhs) const;
};

