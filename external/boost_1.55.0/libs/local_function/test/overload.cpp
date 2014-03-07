
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/config.hpp>
#ifdef BOOST_NO_CXX11_VARIADIC_MACROS
#   error "variadic macros required"
#else

#include <boost/local_function.hpp>
#include <boost/functional/overloaded_function.hpp> // For overloading.
#include <boost/typeof/std/string.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <string>

//[overload_decl
int add_i(int x, int y) { return x + y; }
//]

int main(void) {
    //[overload
    std::string s = "abc";
    std::string BOOST_LOCAL_FUNCTION(
            const bind& s, const std::string& x) {
        return s + x;
    } BOOST_LOCAL_FUNCTION_NAME(add_s)

    double d = 1.23;
    double BOOST_LOCAL_FUNCTION(const bind d, double x, double y, default 0) {
        return d + x + y;
    } BOOST_LOCAL_FUNCTION_NAME(add_d)
    
    boost::overloaded_function<
          std::string (const std::string&)
        , double (double)
        , double (double, double) // Overload giving default param.
        , int (int, int)
    > add(add_s, add_d, add_d, add_i); // Overloaded function object.

    BOOST_TEST(add("xyz") == "abcxyz"); // Call `add_s`.
    BOOST_TEST((4.44 - add(3.21)) <= 0.001); // Call `add_d` (no default).
    BOOST_TEST((44.44 - add(3.21, 40.0)) <= 0.001); // Call `add_d`.
    BOOST_TEST(add(1, 2) == 3); // Call `add_i`.
    //]
    return boost::report_errors();
}

#endif // VARIADIC_MACROS

