// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/object.hpp>

int f(boost::python::object const& x)
{
    x._("hello") = 1;
    return 0;
}
