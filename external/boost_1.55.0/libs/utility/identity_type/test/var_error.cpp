
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/utility/identity_type

#include <map>

//[var_error
#define VAR(type, n) type var ## n

VAR(int, 1);                    // OK.
VAR(std::map<int, char>, 2);    // Error.
//]

int main() { return 0; }

