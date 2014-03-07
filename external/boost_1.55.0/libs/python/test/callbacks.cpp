// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/class.hpp>
#include <boost/ref.hpp>
#include <boost/python/ptr.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/call.hpp>
#include <boost/python/object.hpp>
#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>

using namespace boost::python;
BOOST_STATIC_ASSERT(converter::is_object_manager<handle<> >::value);

int apply_int_int(PyObject* f, int x)
{
    return call<int>(f, x);
}

void apply_void_int(PyObject* f, int x)
{
    call<void>(f, x);
}

struct X
{
    explicit X(int x) : x(x), magic(7654321) { ++counter; }
    X(X const& rhs) : x(rhs.x), magic(7654321) { ++counter; }
    ~X() { BOOST_ASSERT(magic == 7654321); magic = 6666666; x = 9999; --counter; }

    void set(int _x) { BOOST_ASSERT(magic == 7654321); this->x = _x; }
    int value() const { BOOST_ASSERT(magic == 7654321); return x; }
    static int count() { return counter; }
 private:
    void operator=(X const&);
 private:
    int x;
    long magic;
    static int counter;
};

X apply_X_X(PyObject* f, X x)
{
    return call<X>(f, x);
}

void apply_void_X_ref(PyObject* f, X& x)
{
    call<void>(f, boost::ref(x));
}

X& apply_X_ref_handle(PyObject* f, handle<> obj)
{
    return call<X&>(f, obj);
}

X* apply_X_ptr_handle_cref(PyObject* f, handle<> const& obj)
{
    return call<X*>(f, obj);
}

void apply_void_X_cref(PyObject* f, X const& x)
{
    call<void>(f, boost::cref(x));
}

void apply_void_X_ptr(PyObject* f, X* x)
{
    call<void>(f, ptr(x));
}

void apply_void_X_deep_ptr(PyObject* f, X* x)
{
    call<void>(f, x);
}

char const* apply_cstring_cstring(PyObject* f, char const* s)
{
    return call<char const*>(f, s);
}

char const* apply_cstring_pyobject(PyObject* f, PyObject* s)
{
    return call<char const*>(f, borrowed(s));
}

char apply_char_char(PyObject* f, char c)
{
    return call<char>(f, c);
}

char const* apply_to_string_literal(PyObject* f)
{
    return call<char const*>(f, "hello, world");
}

handle<> apply_to_own_type(handle<> x)
{
    // Tests that we can return handle<> from a callback and that we
    // can pass arbitrary handle<T>.
    return call<handle<> >(x.get(), type_handle(borrowed(x->ob_type)));
}

object apply_object_object(PyObject* f, object x)
{
    return call<object>(f, x);
}

int X::counter;

BOOST_PYTHON_MODULE(callbacks_ext)
{
    def("apply_object_object", apply_object_object);
    def("apply_to_own_type", apply_to_own_type);
    def("apply_int_int", apply_int_int);
    def("apply_void_int", apply_void_int);
    def("apply_X_X", apply_X_X);
    def("apply_void_X_ref", apply_void_X_ref);
    def("apply_void_X_cref", apply_void_X_cref);
    def("apply_void_X_ptr", apply_void_X_ptr);
    def("apply_void_X_deep_ptr", apply_void_X_deep_ptr);
        
    def("apply_X_ptr_handle_cref", apply_X_ptr_handle_cref
        , return_value_policy<reference_existing_object>());
        
    def("apply_X_ref_handle", apply_X_ref_handle
        , return_value_policy<reference_existing_object>());
        
    def("apply_cstring_cstring", apply_cstring_cstring);
    def("apply_cstring_pyobject", apply_cstring_pyobject);
    def("apply_char_char", apply_char_char);
    def("apply_to_string_literal", apply_to_string_literal);
        
    
    class_<X>("X", init<int>())
        .def(init<X const&>())
        .def("value", &X::value)
        .def("set", &X::set)
        ;

    def("x_count", &X::count);
}

#include "module_tail.cpp"
