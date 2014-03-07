// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/class.hpp>

struct C {};

struct D {};

struct E
{
   const D fe (const C&)           {return D();}
   const D fe2(const C&, const C&) {return D();}
};

BOOST_PYTHON_MODULE(bienstman2_ext)
{
  using namespace boost::python;

  class_<C>("C");
  class_<D>("D");
  class_<E>("E")
      .def("fe",  &E::fe)  // this compiles.
      .def("fe2", &E::fe2) // this doesn't... well, now it does ;-)
      ;
}
