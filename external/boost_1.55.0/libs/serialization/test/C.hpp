#ifndef BOOST_SERIALIZATION_TEST_A_HPP
#define BOOST_SERIALIZATION_TEST_A_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// C.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/config.hpp>
#include <boost/serialization/traits.hpp>
#include <boost/serialization/split.hpp>

#include "B.hpp"

///////////////////////////////////////////////////////
// Contained class
class C
{
private:
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive &ar, boost::archive::version_type file_version) const;
    template<class Archive>
    void load(Archive & ar, boost::archive::version_type file_version);
    BOOST_SERIALIZATION_MEMBER_SPLIT()
    B b;
public:
    bool operator==(const C &rhs) const;
};

BOOST_CLASS_VERSION(C, 1)

inline bool C::operator==(const C &rhs) const
{
    return b == rhs.b;
}

template<class Archive>
inline void save(Archive &ar, boost::archive::version_type file_version) const
{
    ar << b;
}

template<class Archive>
inline void load(Archive & ar, boost::archive::version_type file_version){
{
    switch(file_version){
    case 1:
        ar >> b;
        break;
    case 2:
    default:
        assert(false);
        break;
    }
}

#endif // BOOST_SERIALIZATION_TEST_C_HPP
