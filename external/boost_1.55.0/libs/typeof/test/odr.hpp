// Copyright (C) 2006 Arkadiy Vertleyb
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

// boostinspect:nounnamed

#include <boost/typeof/typeof.hpp>

struct foo
{
    typedef BOOST_TYPEOF(1 + 2.5) type;
};

namespace
{
    typedef foo::type type;
}
