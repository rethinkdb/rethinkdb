
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

//[goto_error
int error(int x, int y) {
    int BOOST_LOCAL_FUNCTION(int z) {
        if(z <= 0) goto failure;    // Error: Cannot jump to enclosing scope.
        else goto success;          // OK: Can jump within local function.
    success:
        return 0;
    } BOOST_LOCAL_FUNCTION_NAME(validate)

    return validate(x + y);
failure:
    return -1;
}
//]

int main(void) {
    error(1, 2);
    return 0;
}

#endif

