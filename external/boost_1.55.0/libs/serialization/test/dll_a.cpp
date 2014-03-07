/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// dll_a.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Build a dll which contains the serialization for a class A
// used in testing distribution of serialization code in DLLS

#define A_EXPORT
#include "A.hpp"
#include "A.ipp"
#include "A.cpp"

// instantiate code for text archives

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

template
EXPORT_DECL(void) A::serialize(
    boost::archive::text_oarchive &ar,
    const unsigned int /* file_version */
);
template
EXPORT_DECL(void) A::serialize(
    boost::archive::text_iarchive &ar,
    const unsigned int /* file_version */
);

// instantiate code for polymorphic archives

#include <boost/archive/polymorphic_oarchive.hpp>
#include <boost/archive/polymorphic_iarchive.hpp>

template
EXPORT_DECL(void) A::serialize(
    boost::archive::polymorphic_oarchive &ar,
    const unsigned int /* file_version */
);
template
EXPORT_DECL(void) A::serialize(
    boost::archive::polymorphic_iarchive &ar,
    const unsigned int /* file_version */
);

