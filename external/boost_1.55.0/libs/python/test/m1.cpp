// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <boost/python/def.hpp>
#include <boost/python/module.hpp>
#include <boost/python/class.hpp>
#include <boost/python/lvalue_from_pytype.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/manage_new_object.hpp>
#include <boost/python/converter/pytype_function.hpp>
#include <string.h>
#include "simple_type.hpp"
#include "complicated.hpp"

// Declare some straightforward extension types
extern "C" void
dealloc(PyObject* self)
{
    PyObject_Del(self);
}

// Noddy is a type we got from one of the Python sample files
struct NoddyObject : PyObject
{
    int x;
};

PyTypeObject NoddyType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    const_cast<char*>("Noddy"),
    sizeof(NoddyObject),
    0,
    dealloc,    /* tp_dealloc */
    0,          /* tp_print */
    0,          /* tp_getattr */
    0,          /* tp_setattr */
    0,          /* tp_compare */
    0,          /* tp_repr */
    0,          /* tp_as_number */
    0,          /* tp_as_sequence */
    0,          /* tp_as_mapping */
    0,          /* tp_hash */
    0,          /* tp_call */
    0,          /* tp_str */
    0,          /* tp_getattro */
    0,          /* tp_setattro */
    0,          /* tp_as_buffer */
    0,          /* tp_flags */
    0,          /* tp_doc */
    0,          /* tp_traverse */
    0,          /* tp_clear */
    0,          /* tp_richcompare */
    0,          /* tp_weaklistoffset */
    0,          /* tp_iter */
    0,          /* tp_iternext */
    0,          /* tp_methods */
    0,          /* tp_members */
    0,          /* tp_getset */
    0,          /* tp_base */
    0,          /* tp_dict */
    0,          /* tp_descr_get */
    0,          /* tp_descr_set */
    0,          /* tp_dictoffset */
    0,          /* tp_init */
    0,          /* tp_alloc */
    0,          /* tp_new */
    0,          /* tp_free */
    0,          /* tp_is_gc */
    0,          /* tp_bases */
    0,          /* tp_mro */
    0,          /* tp_cache */
    0,          /* tp_subclasses */
    0,          /* tp_weaklist */
#if PYTHON_API_VERSION >= 1012
    0           /* tp_del */
#endif
};

// Create a Noddy containing 42
PyObject* new_noddy()
{
    NoddyObject* noddy = PyObject_New(NoddyObject, &NoddyType);
    noddy->x = 42;
    return (PyObject*)noddy;
}

// Simple is a wrapper around a struct simple, which just contains a char*
struct SimpleObject
{
    PyObject_HEAD
    simple x;
};

struct extract_simple_object
{
    static simple& execute(SimpleObject& o) { return o.x; }
};

PyTypeObject SimpleType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    const_cast<char*>("Simple"),
    sizeof(SimpleObject),
    0,
    dealloc,    /* tp_dealloc */
    0,          /* tp_print */
    0,          /* tp_getattr */
    0,          /* tp_setattr */
    0,          /* tp_compare */
    0,          /* tp_repr */
    0,          /* tp_as_number */
    0,          /* tp_as_sequence */
    0,          /* tp_as_mapping */
    0,          /* tp_hash */
    0,          /* tp_call */
    0,          /* tp_str */
    0,          /* tp_getattro */
    0,          /* tp_setattro */
    0,          /* tp_as_buffer */
    0,          /* tp_flags */
    0,          /* tp_doc */
    0,          /* tp_traverse */
    0,          /* tp_clear */
    0,          /* tp_richcompare */
    0,          /* tp_weaklistoffset */
    0,          /* tp_iter */
    0,          /* tp_iternext */
    0,          /* tp_methods */
    0,          /* tp_members */
    0,          /* tp_getset */
    0,          /* tp_base */
    0,          /* tp_dict */
    0,          /* tp_descr_get */
    0,          /* tp_descr_set */
    0,          /* tp_dictoffset */
    0,          /* tp_init */
    0,          /* tp_alloc */
    0,          /* tp_new */
    0,          /* tp_free */
    0,          /* tp_is_gc */
    0,          /* tp_bases */
    0,          /* tp_mro */
    0,          /* tp_cache */
    0,          /* tp_subclasses */
    0,          /* tp_weaklist */
#if PYTHON_API_VERSION >= 1012
    0           /* tp_del */
#endif
};

// Create a Simple containing "hello, world"
PyObject* new_simple()
{
    SimpleObject* simple = PyObject_New(SimpleObject, &SimpleType);
    simple->x.s = const_cast<char*>("hello, world");
    return (PyObject*)simple;
}

//
// Declare some wrappers/unwrappers to test the low-level conversion
// mechanism. 
//
using boost::python::to_python_converter;

// Wrap a simple by copying it into a Simple
struct simple_to_python
    : to_python_converter<simple, simple_to_python, true>
    //, boost::python::converter::wrap_pytype<&SimpleType>
{
    static PyObject* convert(simple const& x)
    {
        SimpleObject* p = PyObject_New(SimpleObject, &SimpleType);
        p->x = x;
        return (PyObject*)p;
    }
    static PyTypeObject const *get_pytype(){return &SimpleType; }
};

struct int_from_noddy
{
    static int& execute(NoddyObject& p)
    {
        return p.x;
    }
};

//
// Some C++ functions to expose to Python
//

// Returns the length of s's held string
int f(simple const& s)
{
    return strlen(s.s);
}

int f_mutable_ref(simple& s)
{
    return strlen(s.s);
}

int f_mutable_ptr(simple* s)
{
    return strlen(s->s);
}

int f_const_ptr(simple const* s)
{
    return strlen(s->s);
}

int f2(SimpleObject const& s)
{
    return strlen(s.x.s);
}

// A trivial passthru function for simple objects
simple const& g(simple const& x)
{
    return x;
}

struct A
{
    A() : x(0) {}
    virtual ~A() {}
    char const* name() { return "A"; }
    int x;
};

struct B : A
{
    B() : x(1) {}
    static char const* name(B*) { return "B"; }
    int x;
};

struct C : A
{
    C() : x(2) {}
    char const* name() { return "C"; }
    virtual ~C() {}
    int x;
};

struct D : B, C
{
    D() : x(3) {}
    char const* name() { return "D"; }
    int x;
};

A take_a(A const& a) { return a; }
B take_b(B& b) { return b; }
C take_c(C* c) { return *c; }
D take_d(D* const& d) { return *d; }

D take_d_shared_ptr(boost::shared_ptr<D> d) { return *d; }

boost::shared_ptr<A> d_factory() { return boost::shared_ptr<B>(new D); }

struct Unregistered {};
Unregistered make_unregistered(int) { return Unregistered(); }

Unregistered* make_unregistered2(int) { return new Unregistered; }

BOOST_PYTHON_MODULE(m1)
{
    using namespace boost::python;
    using boost::shared_ptr;
    
    simple_to_python();

    lvalue_from_pytype<int_from_noddy,&NoddyType>();

    lvalue_from_pytype<
#if !defined(BOOST_MSVC) || BOOST_MSVC > 1300 // doesn't support non-type member pointer parameters
        extract_member<SimpleObject, simple, &SimpleObject::x>
#else 
        extract_simple_object
#endif 
        , &SimpleType
        >();

    lvalue_from_pytype<extract_identity<SimpleObject>,&SimpleType>();
    
    def("new_noddy", new_noddy);
    def("new_simple", new_simple);

    def("make_unregistered", make_unregistered);
    def("make_unregistered2", make_unregistered2, return_value_policy<manage_new_object>());

      // Expose f() in all its variations
    def("f", f);
    def("f_mutable_ref", f_mutable_ref);
    def("f_mutable_ptr", f_mutable_ptr);
    def("f_const_ptr", f_const_ptr);

    def("f2", f2);
        
      // Expose g()
    def("g", g , return_value_policy<copy_const_reference>()
        );

    def("take_a", take_a);
    def("take_b", take_b);
    def("take_c", take_c);
    def("take_d", take_d);


    def("take_d_shared_ptr", take_d_shared_ptr);
    def("d_factory", d_factory);

    class_<A, shared_ptr<A> >("A")
        .def("name", &A::name)
        ;

    // sequence points don't ensure that "A" is constructed before "B"
    // or "C" below if we make them part of the same chain
    class_<B,bases<A> >("B")
        .def("name", &B::name)
        ;
        
    class_<C,bases<A> >("C")
        .def("name", &C::name)
        ;

    class_<D, bases<B,C> >("D")
        .def("name", &D::name)
        ;

    class_<complicated>("complicated",
                        init<simple const&,int>())
        .def(init<simple const&>())
        .def("get_n", &complicated::get_n)
        ;
}

#include "module_tail.cpp"
