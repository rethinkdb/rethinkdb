// Copyright (C) 2006 Arkadiy Vertleyb
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"

BOOST_STATIC_ASSERT(boost::type_of::test<void(&)()>::value);
BOOST_STATIC_ASSERT(boost::type_of::test<int(&)(int, short)>::value);
