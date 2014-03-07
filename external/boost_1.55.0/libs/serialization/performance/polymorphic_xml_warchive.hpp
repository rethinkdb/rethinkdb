// (C) Copyright 2002-4 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.
#include <boost/archive/polymorphic_xml_woarchive.hpp>
typedef boost::archive::polymorphic_xml_woarchive test_oarchive;
typedef std::wofstream test_ostream;
#include <boost/archive/polymorphic_xml_wiarchive.hpp>
typedef boost::archive::polymorphic_xml_wiarchive test_iarchive;
typedef std::wifstream test_istream;
#define TEST_STREAM_FLAGS (std::ios_base::openmode)0
