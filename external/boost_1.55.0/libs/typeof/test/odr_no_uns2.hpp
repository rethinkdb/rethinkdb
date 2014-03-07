// Copyright (C) 2006 Arkadiy Vertleyb
// Copyright (C) 2006 Peder Holt
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_ODR_NO_UNS2_HPP_INCLUDED
#define BOOST_TYPEOF_ODR_NO_UNS2_HPP_INCLUDED

#define BOOST_TYPEOF_SUPPRESS_UNNAMED_NAMESPACE
#include <boost/typeof/typeof.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

struct odr_test_2
{};

BOOST_TYPEOF_REGISTER_TYPE(odr_test_2)

#endif//BOOST_TYPEOF_ODR_NO_UNS2_HPP_INCLUDED
