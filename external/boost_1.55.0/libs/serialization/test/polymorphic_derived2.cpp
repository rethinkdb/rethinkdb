/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// polymorphic_derived2.cpp   

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/serialization/type_info_implementation.hpp>
#include <boost/serialization/extended_type_info_no_rtti.hpp>
#include <boost/serialization/export.hpp>

#define POLYMORPHIC_DERIVED2_EXPORT
#include "polymorphic_derived2.hpp"

template<class Archive>
void polymorphic_derived2::serialize(
    Archive &ar, 
    const unsigned int /* file_version */
){
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(polymorphic_base);
}

// instantiate code for text archives
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

template EXPORT_DECL(void) polymorphic_derived2::serialize(
    boost::archive::text_oarchive & ar,
    const unsigned int version
);
template EXPORT_DECL(void) polymorphic_derived2::serialize(
    boost::archive::text_iarchive & ar,
    const unsigned int version
);

// instantiate code for polymorphic archives
#include <boost/archive/polymorphic_iarchive.hpp>
#include <boost/archive/polymorphic_oarchive.hpp>

template EXPORT_DECL(void) polymorphic_derived2::serialize(
    boost::archive::polymorphic_oarchive & ar,
    const unsigned int version
);
template EXPORT_DECL(void) polymorphic_derived2::serialize(
    boost::archive::polymorphic_iarchive & ar,
    const unsigned int version
);

// MWerks users can do this to make their code work
BOOST_SERIALIZATION_MWERKS_BASE_AND_DERIVED(polymorphic_base, polymorphic_derived2)

// note: export has to be AFTER #includes for all archive classes
BOOST_CLASS_EXPORT_IMPLEMENT(polymorphic_derived2)

#if 0
#include <boost/serialization/factory.hpp>
BOOST_SERIALIZATION_FACTORY_0(polymorphic_derived2)

template
EXPORT_DECL(polymorphic_derived2 *)
boost::serialization::factory<polymorphic_derived2, 0>(std::va_list ap);
#endif
