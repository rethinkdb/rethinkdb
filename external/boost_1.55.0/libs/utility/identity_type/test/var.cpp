
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/utility/identity_type

#include <map>

#define VAR(type, n) type var ## n

VAR(int, 1); // OK.

//[var_typedef
typedef std::map<int, char> map_type;
VAR(map_type, 3); // OK.
//]

//[var_ok
#include <boost/utility/identity_type.hpp>

VAR(BOOST_IDENTITY_TYPE((std::map<int, char>)), 4); // OK.
//]

int main() { return 0; }

