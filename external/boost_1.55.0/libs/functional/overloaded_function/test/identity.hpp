
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/functional/overloaded_function

#ifndef IDENTITY_HPP_
#define IDENTITY_HPP_

//[identity_typeof
#include <boost/typeof/std/string.hpp> // No need to register `boost::function`.
//]
#include <boost/function.hpp>
#include <string>

//[identity_decls
const std::string& identity_s(const std::string& x) // Function (as pointer).
    { return x; }

int identity_i_impl(int x) { return x; }
int (&identity_i)(int) = identity_i_impl; // Function reference.

double identity_d_impl(double x) { return x; }
boost::function<double (double)> identity_d = identity_d_impl; // Functor.
//]

#endif // #include guard

