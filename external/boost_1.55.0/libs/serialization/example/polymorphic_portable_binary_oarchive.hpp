#ifndef BOOST_ARCHIVE_POLYMORPHIC_PORTABLE_BINARY_OARCHIVE_HPP
#define BOOST_ARCHIVE_POLYMORPHIC_PORTABLE_BINARY_OARCHIVE_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// polymorphic_portable_binary_oarchive.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/archive/detail/polymorphic_oarchive_dispatch.hpp>
#include "portable_binary_oarchive.hpp"

typedef boost::archive::detail::polymorphic_oarchive_dispatch<
    portable_binary_oarchive
 > polymorphic_portable_binary_oarchive;

#include <boost/version.hpp>
#if BOOST_VERSION > 103401
    // required by export
    BOOST_SERIALIZATION_REGISTER_ARCHIVE(
        polymorphic_portable_binary_oarchive
    )
#endif

#endif // BOOST_ARCHIVE_POLYMORPHIC_PORTABLE_BINARY_OARCHIVE_HPP
