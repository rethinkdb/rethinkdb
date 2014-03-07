// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/class.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/return_value_policy.hpp>

struct A {};

struct V 
{
 virtual ~V() {}; // silence compiler warningsa
 virtual void f() = 0;

 const A* inside() {return &a;}

 A a;
};

const A* outside(const V& v) {return &v.a;}

BOOST_PYTHON_MODULE(bienstman1_ext)
{
  using namespace boost::python;
  using boost::shared_ptr;
  using boost::python::return_value_policy;
  using boost::python::reference_existing_object;

  class_<A>("A");

  class_<V, boost::noncopyable>("V", no_init)
      .def("inside", &V::inside, 
           return_value_policy<reference_existing_object>())
      .def("outside", outside, 
           return_value_policy<reference_existing_object>())
      ;
}

