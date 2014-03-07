// Copyright Ralf W. Grosse-Kunstleve 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
    This example shows how to enable pickling without using the
    pickle_suite. The pickling interface (__getinitargs__) is
    implemented in Python.

    For more information refer to boost/libs/python/doc/pickle.html.
 */

#include <boost/python/module.hpp>
#include <boost/python/class.hpp>

#include <string>

namespace boost_python_test {

  // A friendly class.
  class world
  {
    private:
      std::string country;
    public:
      world(const std::string& _country) {
        this->country = _country;
      }
      std::string greet() const { return "Hello from " + country + "!"; }
      std::string get_country() const { return country; }
  };

}

BOOST_PYTHON_MODULE(pickle4_ext)
{
  using namespace boost::python;
  using namespace boost_python_test;
  class_<world>("world", init<const std::string&>())
      .enable_pickling()
      .def("greet", &world::greet)
      .def("get_country", &world::get_country)
      ;
}
