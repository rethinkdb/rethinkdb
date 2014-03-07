// Copyright (C) 2006 Arkadiy Vertleyb
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

struct x{};
BOOST_TYPEOF_REGISTER_TYPE(x)

BOOST_STATIC_ASSERT(boost::type_of::test<x*>::value);
BOOST_STATIC_ASSERT(boost::type_of::test<x&>::value);
BOOST_STATIC_ASSERT(boost::type_of::test<x[20]>::value);
BOOST_STATIC_ASSERT(boost::type_of::test<const x>::value);
BOOST_STATIC_ASSERT(boost::type_of::test<volatile x>::value);
BOOST_STATIC_ASSERT(boost::type_of::test<volatile const x>::value);
BOOST_STATIC_ASSERT(boost::type_of::test<const x&>::value);
BOOST_STATIC_ASSERT(boost::type_of::test<const x*>::value);

