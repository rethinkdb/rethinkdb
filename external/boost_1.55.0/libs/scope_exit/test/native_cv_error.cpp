
// Copyright (C) 2006-2009, 2012 Alexander Nasonov
// Copyright (C) 2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/scope_exit

#include <boost/scope_exit.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/typeof/std/string.hpp>
#include <string>

int main(void) {
    std::string const volatile s;
    BOOST_SCOPE_EXIT( (&s) ) {
        s = "";
    } BOOST_SCOPE_EXIT_END
}

