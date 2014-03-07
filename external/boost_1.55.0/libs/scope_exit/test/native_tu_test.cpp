
// Copyright (C) 2006-2009, 2012 Alexander Nasonov
// Copyright (C) 2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/scope_exit

#include "tu_test.hpp"
#include <boost/detail/lightweight_test.hpp>

void test(void) {
    BOOST_TEST(tu1() == 1);
    BOOST_TEST(tu2() == 2);
}

int main(void) {
    test();
    return boost::report_errors();
}

