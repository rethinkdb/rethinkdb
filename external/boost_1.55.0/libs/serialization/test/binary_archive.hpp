// (C) Copyright 2002-4 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.
// (C) Copyright 2002-4 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

// binary_archive
#include <boost/archive/binary_oarchive.hpp>
typedef boost::archive::binary_oarchive test_oarchive;
typedef std::ofstream test_ostream;

#include <boost/archive/binary_iarchive.hpp>
typedef boost::archive::binary_iarchive test_iarchive;
typedef std::ifstream test_istream;

//#define TEST_STREAM_FLAGS (std::ios::binary | std::ios::in | std::ios::out)
#define TEST_STREAM_FLAGS (std::ios::binary)
