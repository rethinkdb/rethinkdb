// Copyright David Abrahams 2005. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/class.hpp>
#include <boost/utility.hpp>

/*  Non-modifiable definitions */

class basic {
public:
  basic() { name = "cltree.basic"; }
  std::string repr() { return name+"()"; }
protected:
  std::string name;
};

class constant: public basic {
public:
  constant() { name = "cltree.constant"; }
};

class symbol: public basic {
public:
  symbol() { name = "cltree.symbol"; }
};

class variable: public basic {
public:
  variable() { name = "cltree.variable"; }
};

/*  EOF: Non-modifiable definitions */

class symbol_wrapper: public symbol {
public:
  symbol_wrapper(PyObject* /*self*/): symbol() { 
    name = "cltree.wrapped_symbol"; 
  }
};

class variable_wrapper: public variable {
public:
  variable_wrapper(PyObject* /*self*/): variable() { 
    name = "cltree.wrapped_variable";
  }

  // This constructor is introduced only because cannot use
  // boost::noncopyable, see below.
  variable_wrapper(PyObject* /*self*/,variable v): variable(v) {} 

};

BOOST_PYTHON_MODULE(cltree)
{
    boost::python::class_<basic>("basic")
        .def("__repr__",&basic::repr)
        ;

    boost::python::class_<constant, boost::python::bases<basic>, boost::noncopyable>("constant")
        ;


    boost::python::class_<symbol, symbol_wrapper, boost::noncopyable>("symbol")
        ;

    boost::python::class_<variable, boost::python::bases<basic>, variable_wrapper>("variable")
        ;
}

#include "module_tail.cpp"

