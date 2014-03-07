#ifndef POLYMORPHIC_DERIVED2_HPP
#define POLYMORPHIC_DERIVED2_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// polymorphic_derived2.hpp    simple class test

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/type_info_implementation.hpp>
#include <boost/serialization/extended_type_info_typeid.hpp>

#include <boost/preprocessor/empty.hpp>

#include "polymorphic_base.hpp"

class DLL_DECL(BOOST_PP_EMPTY()) polymorphic_derived2 : 
    public polymorphic_base
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(
        Archive &ar, 
        const unsigned int /* file_version */
    );
    virtual const char * get_key() const {
        return "polymorphic_derived2";
    }
};

// we use this because we want to assign a key to this type
// but we don't want to explicitly instantiate code every time
// we do so!!!.  If we don't do this, we end up with the same
// code in BOTH the DLL which implements polymorphic_derived2
// as well as the main program.
BOOST_CLASS_EXPORT_KEY(polymorphic_derived2)

// note the mixing of type_info systems is supported.
BOOST_CLASS_TYPE_INFO(
    polymorphic_derived2,
    boost::serialization::extended_type_info_typeid<polymorphic_derived2>
)

#undef DLL_DECL

#endif // POLYMORPHIC_DERIVED2_HPP

