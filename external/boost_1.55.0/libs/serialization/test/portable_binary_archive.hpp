// (C) Copyright 2002-4 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.
// file includes for testing a custom archive.
// as an example this tests the portable binary archive

#include <fstream>

// #include output archive header
#include "../example/portable_binary_oarchive.hpp"
// define output archive class to be used 
typedef portable_binary_oarchive test_oarchive;
// and corresponding stream
typedef std::ofstream test_ostream;

// repeat the above for correspondng input archive
#include "../example/portable_binary_iarchive.hpp"
typedef portable_binary_iarchive test_iarchive;
typedef std::ifstream test_istream;

// since this archive class isn't compiled into the 
// boost serialization library, include this here
// so that things get instantiated
#include "../example/portable_binary_oarchive.cpp"
#include "../example/portable_binary_iarchive.cpp"

// and stream open flags
#define TEST_STREAM_FLAGS std::ios::binary
