// Copyright (C) 2006 Arkadiy Vertleyb
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

struct A;
BOOST_TYPEOF_REGISTER_TYPE(A)

BOOST_STATIC_ASSERT(boost::type_of::test<A>::value);
