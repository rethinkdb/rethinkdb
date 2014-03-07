/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// polymorphic_derived1.cpp   

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/serialization/type_info_implementation.hpp>
#include <boost/serialization/export.hpp>

#include "polymorphic_derived1.hpp"

const char * polymorphic_derived1::get_key() const {
    return 
        boost::serialization::type_info_implementation<
            polymorphic_derived1
        >::type::get_const_instance().get_key();
}

BOOST_CLASS_EXPORT_IMPLEMENT(polymorphic_derived1)
