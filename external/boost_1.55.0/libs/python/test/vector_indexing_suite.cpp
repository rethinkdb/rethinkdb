// Copyright Joel de Guzman 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/implicit.hpp>

using namespace boost::python;

struct X // a container element
{
    std::string s;
    X():s("default") {}
    X(std::string s):s(s) {}
    std::string repr() const { return s; }
    void reset() { s = "reset"; }
    void foo() { s = "foo"; }
    bool operator==(X const& x) const { return s == x.s; }
    bool operator!=(X const& x) const { return s != x.s; }
};

std::string x_value(X const& x)
{
    return "gotya " + x.s;
}

BOOST_PYTHON_MODULE(vector_indexing_suite_ext)
{    
    class_<X>("X")
        .def(init<>())
        .def(init<X>())
        .def(init<std::string>())
        .def("__repr__", &X::repr)
        .def("reset", &X::reset)
        .def("foo", &X::foo)
    ;

    def("x_value", x_value);
    implicitly_convertible<std::string, X>();
    
    class_<std::vector<X> >("XVec")
        .def(vector_indexing_suite<std::vector<X> >())
    ;
        
    // Compile check only...
    class_<std::vector<float> >("FloatVec")
        .def(vector_indexing_suite<std::vector<float> >())
    ;
    
    // Compile check only...
    class_<std::vector<bool> >("BoolVec")
        .def(vector_indexing_suite<std::vector<bool> >())
    ;
    
    // vector of strings
    class_<std::vector<std::string> >("StringVec")
        .def(vector_indexing_suite<std::vector<std::string> >())
    ;
}

