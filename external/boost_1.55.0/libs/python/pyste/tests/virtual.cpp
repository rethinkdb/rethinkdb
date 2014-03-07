/* Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
 distribution is subject to the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or copy at 
 http://www.boost.org/LICENSE_1_0.txt) 
 */

// Includes ====================================================================
#include <boost/python.hpp>
#include <virtual.h>

// Using =======================================================================
using namespace boost::python;

// Declarations ================================================================


namespace  {


struct virtual_C_Wrapper: virtual_::C
{
    virtual_C_Wrapper(PyObject* self_, const virtual_::C & p0):
        virtual_::C(p0), self(self_) {}

    virtual_C_Wrapper(PyObject* self_):
        virtual_::C(), self(self_) {}

    int f() {
        return call_method< int >(self, "f");
    }

    int default_f() {
        return virtual_::C::f();
    }

    void bar(int p0) {
        call_method< void >(self, "bar", p0);
    }

    void default_bar(int p0) {
        virtual_::C::bar(p0);
    }

    void bar(char * p0) {
        call_method< void >(self, "bar", p0);
    }

    void default_bar(char * p0) {
        virtual_::C::bar(p0);
    }

    int f_abs() {
        return call_method< int >(self, "f_abs");
    }

    PyObject* self;
};



}// namespace 


// Module ======================================================================
BOOST_PYTHON_MODULE(virtual)
{
    class_< virtual_::C, boost::noncopyable, virtual_C_Wrapper >("C", init<  >())
        .def("get_name", &virtual_::C::get_name)
        .def("f", &virtual_::C::f, &virtual_C_Wrapper::default_f)
        .def("bar", (void (virtual_::C::*)(int) )&virtual_::C::bar, (void (virtual_C_Wrapper::*)(int))&virtual_C_Wrapper::default_bar)
        .def("bar", (void (virtual_::C::*)(char *) )&virtual_::C::bar, (void (virtual_C_Wrapper::*)(char *))&virtual_C_Wrapper::default_bar)
    ;

    def("call_f", &virtual_::call_f);
}
