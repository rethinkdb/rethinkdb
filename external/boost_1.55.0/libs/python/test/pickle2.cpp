// Copyright Ralf W. Grosse-Kunstleve 2002-2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
    This example shows how to make an Extension Class "pickleable".

    The world class below contains member data (secret_number) that
    cannot be restored by any of the constructors. Therefore it is
    necessary to provide the __getstate__/__setstate__ pair of pickle
    interface methods.

    For simplicity, the __dict__ is not included in the result of
    __getstate__. This is not generally recommended, but a valid
    approach if it is anticipated that the object's __dict__ will
    always be empty. Note that safety guards are provided to catch
    the cases where this assumption is not true.

    pickle3.cpp shows how to include the object's __dict__ in the
    result of __getstate__.

    For more information refer to boost/libs/python/doc/pickle.html.
 */

#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/class.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/extract.hpp>

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
    getstate(const world& w)
    {
        return boost::python::make_tuple(w.get_secret_number());
    }

    static
    void
    setstate(world& w, boost::python::tuple state)
    {
        using namespace boost::python;
        if (len(state) != 1)
        {
          PyErr_SetObject(PyExc_ValueError,
                          ("expected 1-item tuple in call to __setstate__; got %s"
                           % state).ptr()
              );
          throw_error_already_set();
        }

        long number = extract<long>(state[0]);
        if (number != 42)
            w.set_secret_number(number);
    }
  };

}

BOOST_PYTHON_MODULE(pickle2_ext)
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
