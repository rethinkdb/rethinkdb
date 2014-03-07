// (C) Copyright 2006 Douglas Gregor <doug.gregor -at- gmail.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor

#include <boost/parallel/mpi/python.hpp>
#include <boost/python.hpp>
#include <boost/serialization/list.hpp>
using namespace boost::python;

template<typename T>
boost::python::list list_to_python(const std::list<T>& value) {
  boost::python::list result;
  for (typename std::list<T>::const_iterator i = value.begin();
       i != value.end(); ++i)
    result.append(*i);
  return result;
}

BOOST_PYTHON_MODULE(skeleton_content)
{
  using boost::python::arg;

  class_<std::list<int> >("list_int")
    .def("push_back", &std::list<int>::push_back, arg("value"))
    .def("pop_back", &std::list<int>::pop_back)
    .def("reverse", &std::list<int>::reverse)
    .def(boost::python::self == boost::python::self)
    .def(boost::python::self != boost::python::self)
    .add_property("size", &std::list<int>::size)
    .def("to_python", &list_to_python<int>);

  boost::parallel::mpi::python::register_skeleton_and_content<std::list<int> >();
}
