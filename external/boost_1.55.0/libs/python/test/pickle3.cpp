// Copyright Ralf W. Grosse-Kunstleve 2002-2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
    This example shows how to make an Extension Class "pickleable".

    The world class below contains member data (secret_number) that
    cannot be restored by any of the constructors. Therefore it is
    necessary to provide the __getstate__/__setstate__ pair of pickle
    interface methods.

    The object's __dict__ is included in the result of __getstate__.
    This requires more code (compare with pickle2.cpp), but is
    unavoidable if the object's __dict__ is not always empty.

    For more information refer to boost/libs/python/doc/pickle.html.
 */

#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/class.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/back_reference.hpp>

namespace boost_python_test {

  // A friendly class.
  class world
  {
    public:
      world(const std::string& _country) : secret_number(0) {
        this->country = _country;
      }
      std::string greet() const { return "Hello from " + country + "!"; }
      std::string get_country() const { return country; }
      void set_secret_number(int number) { secret_number = number; }
      int get_secret_number() const { return secret_number; }
    private:
      std::string country;
      int secret_number;
  };

  struct world_pickle_suite : boost::python::pickle_suite
  {
    static
    boost::python::tuple
    getinitargs(const world& w)
    {
        return boost::python::make_tuple(w.get_country());
    }

    static
    boost::python::tuple
    getstate(boost::python::object w_obj)
    {
        world const& w = boost::python::extract<world const&>(w_obj)();

        return boost::python::make_tuple(
            w_obj.attr("__dict__"),
            w.get_secret_number());
    }

    static
    void
    setstate(boost::python::object w_obj, boost::python::tuple state)
    {
        using namespace boost::python;
        world& w = extract<world&>(w_obj)();

        if (len(state) != 2)
        {
          PyErr_SetObject(PyExc_ValueError,
                          ("expected 2-item tuple in call to __setstate__; got %s"
                           % state).ptr()
              );
          throw_error_already_set();
        }

        // restore the object's __dict__
        dict d = extract<dict>(w_obj.attr("__dict__"))();
        d.update(state[0]);

        // restore the internal state of the C++ object
        long number = extract<long>(state[1]);
        if (number != 42)
            w.set_secret_number(number);
    }

    static bool getstate_manages_dict() { return true; }
  };

}

BOOST_PYTHON_MODULE(pickle3_ext)
{
    using namespace boost_python_test;
    boost::python::class_<world>(
        "world", boost::python::init<const std::string&>())
        .def("greet", &world::greet)
        .def("get_secret_number", &world::get_secret_number)
        .def("set_secret_number", &world::set_secret_number)
        .def_pickle(world_pickle_suite())
        ;
}
