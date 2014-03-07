/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// xml_iarchive.cpp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#if (defined _MSC_VER) && (_MSC_VER == 1200)
#  pragma warning (disable : 4786) // too long name, harmless warning
#endif

#define BOOST_ARCHIVE_SOURCE

// the following works around an issue between spirit 1.61 and borland.
// it turns out the the certain spirit stuff must be defined before
// certain parts of mpl.  including this here makes sure that happens
#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#if BOOST_WORKAROUND(__BORLANDC__, <= 0x560 )
#include <boost/archive/impl/basic_xml_grammar.hpp>
#endif

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/detail/archive_serializer_map.hpp>

// explicitly instantiate for this type of xml stream
#include <boost/archive/impl/archive_serializer_map.ipp>
#include <boost/archive/impl/basic_xml_iarchive.ipp>
#include <boost/archive/impl/xml_iarchive_impl.ipp>

namespace boost {
namespace archive {

template class detail::archive_serializer_map<naked_xml_iarchive>;
template class basic_xml_iarchive<naked_xml_iarchive> ;
template class xml_iarchive_impl<naked_xml_iarchive> ;

template class detail::archive_serializer_map<xml_iarchive>;
template class basic_xml_iarchive<xml_iarchive> ;
template class xml_iarchive_impl<xml_iarchive> ;

} // namespace archive
} // namespace boost
