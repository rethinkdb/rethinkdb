// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This module exercises the converters exposed in m1 at a low level
// by exposing raw Python extension functions that use wrap<> and
// unwrap<> objects.
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/copy_non_const_reference.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/return_value_policy.hpp>
#include "simple_type.hpp"

#if PY_VERSION_HEX >= 0x03000000
# define PyString_FromString PyUnicode_FromString
# define PyInt_FromLong PyLong_FromLong
#endif

// Get a simple (by value) from the argument, and return the
// string it holds.
PyObject* unwrap_simple(simple x)
{
    return PyString_FromString(x.s);
}

// Likewise, but demands that its possible to get a non-const
// reference to the simple.
PyObject* unwrap_simple_ref(simple& x)
{
    return PyString_FromString(x.s);
}

// Likewise, with a const reference to the simple object.
PyObject* unwrap_simple_const_ref(simple const& x)
{
    return PyString_FromString(x.s);
}

// Get an int (by value) from the argument, and convert it to a
// Python Int.
PyObject* unwrap_int(int x)
{
    return PyInt_FromLong(x);
}

// Get a non-const reference to an int from the argument
PyObject* unwrap_int_ref(int& x)
{
    return PyInt_FromLong(x);
}

// Get a const reference to an  int from the argument.
PyObject* unwrap_int_const_ref(int const& x)
{
    return PyInt_FromLong(x);
}

#if PY_VERSION_HEX >= 0x03000000
# undef PyString_FromString
# undef PyInt_FromLong
#endif

// rewrap<T> extracts a T from the argument, then converts the T back
// to a PyObject* and returns it.
template <class T>
struct rewrap
{
    static T f(T x) { return x; }
};

BOOST_PYTHON_MODULE(m2)
{
    using boost::python::return_value_policy;
    using boost::python::copy_const_reference;
    using boost::python::copy_non_const_reference;
    using boost::python::def;
    
    def("unwrap_int", unwrap_int);
    def("unwrap_int_ref", unwrap_int_ref);
    def("unwrap_int_const_ref", unwrap_int_const_ref);
    def("unwrap_simple", unwrap_simple);
    def("unwrap_simple_ref", unwrap_simple_ref);
    def("unwrap_simple_const_ref", unwrap_simple_const_ref);

    def("wrap_int", &rewrap<int>::f);
        
    def("wrap_int_ref", &rewrap<int&>::f
        , return_value_policy<copy_non_const_reference>()
        );
        
    def("wrap_int_const_ref", &rewrap<int const&>::f
        , return_value_policy<copy_const_reference>()
        );
        
    def("wrap_simple", &rewrap<simple>::f);
        
    def("wrap_simple_ref", &rewrap<simple&>::f
        , return_value_policy<copy_non_const_reference>()
        );
        
    def("wrap_simple_const_ref", &rewrap<simple const&>::f
        , return_value_policy<copy_const_reference>()
            );
}

#include "module_tail.cpp"
