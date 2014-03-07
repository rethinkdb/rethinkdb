// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>

#if defined(_AIX) && defined(__EDG_VERSION__) && __EDG_VERSION__ < 245
# include <iostream> // works around a KCC intermediate code generation bug
#endif

BOOST_PYTHON_MODULE(minimal_ext)
{
}

#include "module_tail.cpp"
