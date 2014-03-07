//  intmax_c.cpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#include <boost/cstdint.hpp>

#ifdef INTMAX_C
#define BOOST_INTMAX_C(a) INTMAX_C(a)
#else
#define BOOST_INTMAX_C(a) a##LL
#endif

boost::intmax_t i = BOOST_INTMAX_C(1000000000);
int main() {
    return (i);
}
