// Copyright (C) 2006 Arkadiy Vertleyb
// Copyright (C) 2006 Peder Holt
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_ODR_NO_UNS1_HPP_INCLUDED
#define BOOST_TYPEOF_ODR_NO_UNS1_HPP_INCLUDED

#define BOOST_TYPEOF_SUPPRESS_UNNAMED_NAMESPACE
#include <boost/typeof/typeof.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

struct odr_test_1
{};

BOOST_TYPEOF_REGISTER_TYPE(odr_test_1)

#endif//BOOST_TYPEOF_ODR_NO_UNS1_HPP_INCLUDED
