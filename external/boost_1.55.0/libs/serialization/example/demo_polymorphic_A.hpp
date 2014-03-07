#ifndef BOOST_SERIALIZATION_EXAMPLE_DEMO_POLYMORPHIC_A_HPP
#define BOOST_SERIALIZATION_EXAMPLE_DEMO_POLYMORPHIC_A_HPP

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// demo_polymorphic_A.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

namespace boost {
namespace archive {

class polymorphic_iarchive;
class polymorphic_oarchive;

} // namespace archive
} // namespace boost

struct A {
    // class a contains a pointer to a "hidden" declaration
    template<class Archive>
    void serialize(
        Archive & ar, 
        const unsigned int file_version
    ){
        ar & data;
    }
    int data;
    bool operator==(const A & rhs) const {
        return data == rhs.data;
    }
    A() :
        data(0)
    {}
};

#endif // BOOST_SERIALIZATION_EXAMPLE_DEMO_POLYMORPHIC_A_HPP
