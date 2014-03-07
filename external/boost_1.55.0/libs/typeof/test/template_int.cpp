// Copyright (C) 2006 Arkadiy Vertleyb
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include <climits>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

struct x;
BOOST_TYPEOF_REGISTER_TYPE(x)

template<
    class T, char c, unsigned short us, int i, unsigned long ul, 
    bool b1, bool b2, signed char sc, unsigned u> 
    struct Tpl
{};

BOOST_TYPEOF_REGISTER_TEMPLATE(Tpl, 
    (class)
    (char)
    (unsigned short)
    (int)
    (unsigned long)
    (bool)
    (bool)
    (signed char)
    (unsigned)
    )

BOOST_STATIC_ASSERT((boost::type_of::test<Tpl<int, 5, 4, -3, 2, true, false, -1, 5> >::value));
BOOST_STATIC_ASSERT((boost::type_of::test<Tpl<int, 1, 1, 0, ULONG_MAX, false, true, -1, 0> >::value));
