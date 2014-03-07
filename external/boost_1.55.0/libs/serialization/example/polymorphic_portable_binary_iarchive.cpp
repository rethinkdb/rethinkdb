/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// polymorphic_portable_binary_iarchive.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <istream>

#define BOOST_ARCHIVE_SOURCE
#include "polymorphic_portable_binary_iarchive.hpp"

// explicitly instantiate for this type of text stream
#include <boost/archive/impl/basic_binary_iarchive.ipp>
#include <boost/archive/impl/archive_pointer_iserializer.ipp>
#include <boost/archive/impl/basic_binary_iprimitive.ipp>

namespace boost {
namespace archive {

template class binary_iarchive_impl<
    polymorphic_portable_binary_iarchive, 
    std::istream::char_type, 
    std::istream::traits_type
>;
template class detail::archive_pointer_iserializer<
    polymorphic_portable_binary_iarchive
> ;

} // namespace archive
} // namespace boost
