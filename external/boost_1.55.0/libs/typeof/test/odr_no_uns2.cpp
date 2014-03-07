// Copyright (C) 2006 Arkadiy Vertleyb
// Copyright (C) 2006 Peder Holt
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#include "odr_no_uns2.hpp"
#include "odr_no_uns1.hpp"

void odr_no_uns2()
{
    odr_test_1 t1;
    odr_test_2 t2;
    BOOST_AUTO(v1, t1);
    BOOST_AUTO(v2, t2);
}
