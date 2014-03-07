// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/converter/as_to_python_function.hpp>

struct hopefully_illegal
{
    static PyObject* convert(int&);
};

PyObject* x = boost::python::converter::as_to_python_function<int, hopefully_illegal>::convert(0);
