// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/class.hpp>
#include <boost/python/module.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/return_by_value.hpp>
#include "test_class.hpp"

#if defined(_AIX) && defined(__EDG_VERSION__) && __EDG_VERSION__ < 245
# include <iostream> // works around a KCC intermediate code generation bug
#endif


using namespace boost::python;

typedef test_class<> X;

struct Y : test_class<1>
{
    Y(int v) : test_class<1>(v) {}
    Y& operator=(Y const& rhs) { x = rhs.x; return *this; }
    bool q;
};

double get_fair_value(X const& x) { return x.value(); }


struct VarBase
{
    VarBase(std::string name_) : name(name_) {}
    
    std::string const name;
    std::string get_name1() const { return name; }
    
};

struct Var : VarBase
{
    Var(std::string name_) : VarBase(name_), value(), name2(name.c_str()), y(6) {}
    std::string const& get_name2() const { return name; }
    float value;
    char const* name2;
    Y y;

    static int static1;
    static Y static2;
};

int Var::static1 = 0;
Y Var::static2(0);

// Compilability regression tests
namespace boost_python_test
{
  struct trivial
  {
    trivial() : value(123) {}
    double value;
  };

  struct Color3
  {
    static const Color3 black;
  };

  const Color3 Color3::black
#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
  = {}
#endif 
      ;

  void compilability_test()
  {
    class_<trivial>("trivial")
      .add_property("property", make_getter(&trivial::value, return_value_policy<return_by_value>()))
      .def_readonly("readonly", &trivial::value)
    ;

    class_< Color3 >("Color3", init< const Color3 & >())
        .def_readonly("BLACK", &Color3::black)   // line 17
        ;
  }
}

BOOST_PYTHON_MODULE(data_members_ext)
{
    using namespace boost_python_test;
    class_<X>("X", init<int>())
        .def("value", &X::value)
        .def("set", &X::set)
        .def_readonly("x", &X::x)
        .add_property("fair_value", get_fair_value)
        ;

    class_<Y>("Y", init<int>())
        .def("value", &Y::value)
        .def("set", &Y::set)
        .def_readwrite("x", &Y::x)
        .def_readwrite("q", &Y::q)
        ;

    class_<Var>("Var", init<std::string>())
        .def_readonly("name", &Var::name)
        .def_readonly("name2", &Var::name2)
        .def_readwrite("value", &Var::value)
        .def_readonly("y", &Var::y)
        
        // Test return_by_value for plain values and for
        // pointers... return_by_value was implemented as a
        // side-effect of implementing data member support, so it made
        // sense to add the test here.
        .def("get_name1", &Var::get_name1, return_value_policy<return_by_value>())
        .def("get_name2", &Var::get_name2, return_value_policy<return_by_value>())
        
        .add_property("name3", &Var::get_name1)

        // Test static data members
        .def_readonly("ro1a", &Var::static1)
        .def_readonly("ro1b", Var::static1)
        .def_readwrite("rw1a", &Var::static1)
        .def_readwrite("rw1b", Var::static1)

        .def_readonly("ro2a", &Var::static2)
        .def_readonly("ro2b", Var::static2)
        .def_readwrite("rw2a", &Var::static2)
        .def_readwrite("rw2b", Var::static2)
        ;
}

#include "module_tail.cpp"
