// Copyright Joel de Guzman 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/suite/indexing/map_indexing_suite.hpp>
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


BOOST_PYTHON_MODULE(map_indexing_suite_ext)
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

    class_<std::map<std::string, X> >("XMap")
        .def(map_indexing_suite<std::map<std::string, X> >())
    ;

    void int_map_indexing_suite(); // moved to int_map_indexing_suite.cpp to 
    int_map_indexing_suite();      // avoid MSVC 6/7 internal structure overflow
    
#if 0
    // Compile check only...
    class_<std::map<int, int> >("IntMap")
        .def(map_indexing_suite<std::map<int, int> >())
    ;
#endif 

    // Some more..
    class_<std::map<std::string, boost::shared_ptr<X> > >("TestMap")
        .def(map_indexing_suite<std::map<std::string, boost::shared_ptr<X> >, true>())
    ;

    void a_map_indexing_suite(); // moved to a_map_indexing_suite.cpp to 
    a_map_indexing_suite();      // avoid MSVC 6/7 internal structure overflow

}

#include "module_tail.cpp"
