#ifndef BOOST_SERIALIZATION_TEST_D_HPP
#define BOOST_SERIALIZATION_TEST_D_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// D.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <cstddef> // NULL

#include "test_tools.hpp"
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/serialization/throw_exception.hpp>
#include <boost/serialization/split_member.hpp>

#include "B.hpp"

///////////////////////////////////////////////////////
// Contained class with multiple identical pointers
class D
{
private:
    friend class boost::serialization::access;
    B *b1;
    B *b2;
    template<class Archive>
    void save(Archive &ar, const unsigned int file_version) const{
        ar << BOOST_SERIALIZATION_NVP(b1);
        ar << BOOST_SERIALIZATION_NVP(b2);
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int file_version){
        BOOST_TRY {
            ar >> boost::serialization::make_nvp("b", b1);
            ar >> boost::serialization::make_nvp("b", b2);
        }
        BOOST_CATCH (...){
            // eliminate invalid pointers
            b1 = NULL;
            b2 = NULL;
            BOOST_FAIL( "multiple identical pointers failed to load" );
        }
        BOOST_CATCH_END
        // check that loading was correct
        BOOST_CHECK(b1 == b2);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
public:
    D();
    ~D();
    bool operator==(const D &rhs) const;
};

BOOST_CLASS_VERSION(D, 3)

D::D()
{
    b1 = new B();
    b2 = b1;
}

D::~D()
{
    delete b1;
}

bool D::operator==(const D &rhs) const
{
    if(! (*b1 == *(rhs.b1)) )
        return false;   
    if(! (*b2 == *(rhs.b2)) )
        return false;
    return true;
}

#endif // BOOST_SERIALIZATION_TEST_D_HPP
