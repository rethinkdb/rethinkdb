/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// A.ipp    simple class test

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/detail/workaround.hpp>
#if ! BOOST_WORKAROUND(BOOST_MSVC, <= 1300)

#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
#include <boost/archive/dinkumware.hpp>
#endif

#include "A.hpp"

template<class Archive>
void A::serialize(
    Archive &ar,
    const unsigned int /* file_version */
){
    ar & BOOST_SERIALIZATION_NVP(b);
    #ifndef BOOST_NO_INT64_T
    ar & BOOST_SERIALIZATION_NVP(f);
    ar & BOOST_SERIALIZATION_NVP(g);
    #endif
    #if BOOST_WORKAROUND(__BORLANDC__,  <= 0x551 )
        int i;
        if(BOOST_DEDUCED_TYPENAME Archive::is_saving::value){
            i = l;
            ar & BOOST_SERIALIZATION_NVP(i);
        }
        else{
            ar & BOOST_SERIALIZATION_NVP(i);
            l = i;
        }
    #else
        ar & BOOST_SERIALIZATION_NVP(l);
    #endif
    ar & BOOST_SERIALIZATION_NVP(m);
    ar & BOOST_SERIALIZATION_NVP(n);
    ar & BOOST_SERIALIZATION_NVP(o);
    ar & BOOST_SERIALIZATION_NVP(p);
    ar & BOOST_SERIALIZATION_NVP(q);
    #ifndef BOOST_NO_CWCHAR
    ar & BOOST_SERIALIZATION_NVP(r);
    #endif
    ar & BOOST_SERIALIZATION_NVP(c);
    ar & BOOST_SERIALIZATION_NVP(s);
    ar & BOOST_SERIALIZATION_NVP(t);
    ar & BOOST_SERIALIZATION_NVP(u);
    ar & BOOST_SERIALIZATION_NVP(v);
    ar & BOOST_SERIALIZATION_NVP(w);
    ar & BOOST_SERIALIZATION_NVP(x);
    ar & BOOST_SERIALIZATION_NVP(y);
    #ifndef BOOST_NO_STD_WSTRING
    ar & BOOST_SERIALIZATION_NVP(z);
    #endif
}

#endif // workaround BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
