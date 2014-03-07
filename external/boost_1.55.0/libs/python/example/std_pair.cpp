// Copyright Ralf W. Grosse-Kunstleve 2002-2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/to_python_converter.hpp>

namespace { // Avoid cluttering the global namespace.

  // Converts a std::pair instance to a Python tuple.
  template <typename T1, typename T2>
  struct std_pair_to_tuple
  {
    static PyObject* convert(std::pair<T1, T2> const& p)
    {
      return boost::python::incref(
        boost::python::make_tuple(p.first, p.second).ptr());
    }
    static PyTypeObject const *get_pytype () {return &PyTuple_Type; }
  };

  // Helper for convenience.
  template <typename T1, typename T2>
  struct std_pair_to_python_converter
  {
    std_pair_to_python_converter()
    {
      boost::python::to_python_converter<
        std::pair<T1, T2>,
        std_pair_to_tuple<T1, T2>,
        true //std_pair_to_tuple has get_pytype
        >();
    }
  };

  // Example function returning a std::pair.
  std::pair<int, int>
  foo() { return std::pair<int, int>(3, 5); }

} // namespace anonymous

BOOST_PYTHON_MODULE(std_pair_ext)
{
  using namespace boost::python;
  std_pair_to_python_converter<int, int>();
  def("foo", foo);
}
