// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/module.hpp>
#include <boost/python/class.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/manage_new_object.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/pure_virtual.hpp>
#include <boost/python/wrapper.hpp>
#include <boost/python/def.hpp>
#include <boost/python/call.hpp>
#include <boost/utility.hpp>

#include <memory>

#ifdef HELD_BY_AUTO_PTR
# define HELD_PTR(X) , std::auto_ptr< X >
#else
# define HELD_PTR(X)
#endif 

using namespace boost::python;

struct P
{
    virtual ~P(){}
    virtual char const* f() = 0;
    char const* g() { return "P::g()"; }
};

struct PCallback : P, wrapper<P>
{
    char const* f()
    {
#if BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
        return call<char const*>(this->get_override("f").ptr());
#else 
        return this->get_override("f")();
#endif 
    }
};

struct Q : virtual P
{
    char const* f() { return "Q::f()"; } 
};

struct A
{
    virtual ~A(){}
    virtual char const* f() { return "A::f()"; }
};

struct ACallback :  A, wrapper<A>
{
    char const* f()
    {
        if (override f = this->get_override("f"))
#if BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
            return call<char const*>(f.ptr());
#else 
            return f();
#endif 

        return A::f();
    }

    char const* default_f() { return this->A::f(); }
};

struct B : A
{
    virtual char const* f() { return "B::f()"; } 
};

struct C : A
{
    virtual char const* f() { return "C::f()"; }
};

struct D : A
{
    virtual char const* f() { return "D::f()"; }
    char const* g() { return "D::g()"; }
};

struct DCallback :  D,  wrapper<D>
{
    char const* f()
    {
        if (override f = this->get_override("f"))
#if BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
            return call<char const*>(f.ptr());
#else 
            return f();
#endif 
        //else
            return D::f();
    }
};

    
A& getBCppObj ()
{
    static B b;
    return b;
}

char const* call_f(A& a) { return a.f(); }

A* factory(unsigned choice)
{
    switch (choice % 3)
    {
    case 0: return new A;
        break;
    case 1: return new B;
        break;
    default: return new C;
        break;
    }
}

C& getCCppObj ()
{
    static C c;
    return c;
}

A* pass_a(A* x) { return x; }

#ifdef HELD_BY_AUTO_PTR
BOOST_PYTHON_MODULE_INIT(polymorphism2_auto_ptr_ext)
#else
BOOST_PYTHON_MODULE_INIT(polymorphism2_ext)
#endif 
{
    class_<ACallback HELD_PTR(A),boost::noncopyable>("A")
        .def("f", &A::f, &ACallback::default_f)
        ;
    
    def("getBCppObj", getBCppObj, return_value_policy<reference_existing_object>());

    class_<C HELD_PTR(C),bases<A>,boost::noncopyable>("C")
        .def("f", &C::f)
        ;
    
    class_<DCallback HELD_PTR(D),bases<A>,boost::noncopyable>("D")
        .def("f", &D::f)
        .def("g", &D::g)
        ;

    def("pass_a", &pass_a,  return_internal_reference<>());
    
    def("getCCppObj", getCCppObj, return_value_policy<reference_existing_object>());

    def("factory", factory, return_value_policy<manage_new_object>());

    def("call_f", call_f);

    class_<PCallback,boost::noncopyable>("P")
        .def("f", pure_virtual(&P::f))
        ;

    class_<Q HELD_PTR(Q), bases<P> >("Q")
        .def("g", &P::g) // make sure virtual inheritance doesn't interfere
        ;
}

//#include "module_tail.cpp"
