
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
#include <boost/utility/identity_type.hpp>
#include <boost/typeof/std/string.hpp>  // Type-of registrations
#include <boost/typeof/std/map.hpp>     // needed for `NAME` macro.
#include <map>
#include <string>

std::string cat(const std::string& x, const std::string& y) { return x + y; }

template<typename V, typename K>
struct key_sizeof {
    static int const value;
};

template<typename V, typename K>
int const key_sizeof<V, K>::value = sizeof(K);

typedef int sign_t;

int main(void) {
    //[macro_commas
    void BOOST_LOCAL_FUNCTION(
        BOOST_IDENTITY_TYPE((const std::map<std::string, size_t>&)) m,
        BOOST_IDENTITY_TYPE((::sign_t)) sign,
        const size_t& factor,
                default (key_sizeof<std::string, size_t>::value),
        const std::string& separator, default cat(":", " ")
    ) {
        // Do something...
    } BOOST_LOCAL_FUNCTION_NAME(f)
    //]

    std::map<std::string, size_t> m;
    ::sign_t sign = -1;
    f(m, sign);
    return 0;
}

#endif // VARIADIC_MACROS

