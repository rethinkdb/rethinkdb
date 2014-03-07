// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_UNITS_STRINGIZE_IMPL(x) #x
#define BOOST_UNITS_STRINGIZE(x) BOOST_UNITS_STRINGIZE_IMPL(x)

#define BOOST_UNITS_HEADER BOOST_UNITS_STRINGIZE(BOOST_UNITS_HEADER_NAME)

#include BOOST_UNITS_HEADER
#include BOOST_UNITS_HEADER

int main() {}
