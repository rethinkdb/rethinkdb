// Copyright (C) 2006 Arkadiy Vertleyb
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

template<class T, class U> 
struct t0;

template<class T, int n> 
struct t1;

template<template<class, class> class T, template<class, int> class U> 
struct t2;

BOOST_TYPEOF_REGISTER_TEMPLATE(t0, 2)

BOOST_TYPEOF_REGISTER_TEMPLATE(t1, (class)(int))

BOOST_TYPEOF_REGISTER_TEMPLATE(t2,
                               (BOOST_TYPEOF_TEMPLATE(2))
                               (BOOST_TYPEOF_TEMPLATE((class)(int)))
                               )

BOOST_STATIC_ASSERT((boost::type_of::test<t2<t0, t1> >::value));
