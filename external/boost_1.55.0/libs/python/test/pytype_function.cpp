// Copyright Joel de Guzman 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/python/class.hpp>

using namespace boost::python;

struct A
{
};

struct B
{
  A a;
  B(const A& a_):a(a_){}
};

// Converter from A to python int
struct BToPython
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
 : converter::to_python_target_type<A>  //inherits get_pytype 
#endif
{
  static PyObject* convert(const B& b)
  {
    return boost::python::incref(boost::python::object(b.a).ptr());
  }
};

// Conversion from python int to A
struct BFromPython 
{
  BFromPython()
  {
    boost::python::converter::registry::push_back(
        &convertible,
        &construct,
        boost::python::type_id< B >()
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
        , &converter::expected_from_python_type<A>::get_pytype//convertible to A can be converted to B
#endif
        );
  }

  static void* convertible(PyObject* obj_ptr)
  {
      extract<const A&> ex(obj_ptr);
      if (!ex.check()) return 0;
      return obj_ptr;
  }

  static void construct(
      PyObject* obj_ptr,
      boost::python::converter::rvalue_from_python_stage1_data* data)
  {
    void* storage = (
        (boost::python::converter::rvalue_from_python_storage< B >*)data)-> storage.bytes;

    extract<const A&> ex(obj_ptr);
    new (storage) B(ex());
    data->convertible = storage;
  }
};


B func(const B& b) { return b ; }


BOOST_PYTHON_MODULE(pytype_function_ext)
{
  to_python_converter< B , BToPython,true >(); //has get_pytype
  BFromPython();

  class_<A>("A") ;

  def("func", &func);

}

#include "module_tail.cpp"
