// Copyright 2008 Lubomir Bourdev and Hailin Jin
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <exception>

void error_if(bool condition) {
    if (condition)
        throw std::exception();
}

