// Copyright (C) 2006 Arkadiy Vertleyb
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

struct P1;
struct P2;

template<class P1, class P2> struct Tpl;

BOOST_TYPEOF_REGISTER_TYPE(P1)
BOOST_TYPEOF_REGISTER_TYPE(P2)
BOOST_TYPEOF_REGISTER_TEMPLATE(Tpl, 2)

BOOST_STATIC_ASSERT((boost::type_of::test<Tpl<P1, P2> >::value));
