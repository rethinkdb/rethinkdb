// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/numeric.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/cast.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/detail/raw_pyobject.hpp>
#include <boost/python/extract.hpp>

namespace boost { namespace python { namespace numeric {

namespace
{
  enum state_t { failed = -1, unknown, succeeded };
  state_t state = unknown;
  std::string module_name;
  std::string type_name;

  handle<> array_module;
  handle<> array_type;
  handle<> array_function;

  void throw_load_failure()
  {
      PyErr_Format(
          PyExc_ImportError
          , "No module named '%s' or its type '%s' did not follow the NumPy protocol"
          , module_name.c_str(), type_name.c_str());
      throw_error_already_set();
      
  }

  bool load(bool throw_on_error)
  {
      if (!state)
      {
          if (module_name.size() == 0)
          {
              module_name = "numarray";
              type_name = "NDArray";
              if (load(false))
                  return true;
              module_name = "Numeric";
              type_name = "ArrayType";
          }

          state = failed;
          PyObject* module = ::PyImport_Import(object(module_name).ptr());
          if (module)
          {
              PyObject* type = ::PyObject_GetAttrString(module, const_cast<char*>(type_name.c_str()));

              if (type && PyType_Check(type))
              {
                  array_type = handle<>(type);
                  PyObject* function = ::PyObject_GetAttrString(module, const_cast<char*>("array"));
                  
                  if (function && PyCallable_Check(function))
                  {
                      array_function = handle<>(function);
                      state = succeeded;
                  }
              }
          }
      }
      
      if (state == succeeded)
          return true;
      
      if (throw_on_error)
          throw_load_failure();
      
      PyErr_Clear();
      return false;
  }

  object demand_array_function()
  {
      load(true);
      return object(array_function);
  }
}

void array::set_module_and_type(char const* package_name, char const* type_attribute_name)
{
    state = unknown;
    module_name = package_name ? package_name : "" ;
    type_name = type_attribute_name ? type_attribute_name : "" ;
}

std::string array::get_module_name()
{
    load(false);
    return module_name;
}

namespace aux
{
  bool array_object_manager_traits::check(PyObject* obj)
  {
      if (!load(false))
          return false;
      return ::PyObject_IsInstance(obj, array_type.get());
  }

  python::detail::new_non_null_reference
  array_object_manager_traits::adopt(PyObject* obj)
  {
      load(true);
      return detail::new_non_null_reference(
          pytype_check(downcast<PyTypeObject>(array_type.get()), obj));
  }

  PyTypeObject const* array_object_manager_traits::get_pytype()
  {
      load(false);
      if(!array_type) return 0;
      return downcast<PyTypeObject>(array_type.get());
  }

# define BOOST_PYTHON_AS_OBJECT(z, n, _) object(x##n)
# define BOOST_PP_LOCAL_MACRO(n)                                        \
    array_base::array_base(BOOST_PP_ENUM_PARAMS(n, object const& x))    \
        : object(demand_array_function()(BOOST_PP_ENUM_PARAMS(n, x)))   \
    {}
# define BOOST_PP_LOCAL_LIMITS (1, 6)
# include BOOST_PP_LOCAL_ITERATE()
# undef BOOST_PYTHON_AS_OBJECT

    array_base::array_base(BOOST_PP_ENUM_PARAMS(7, object const& x))
        : object(demand_array_function()(BOOST_PP_ENUM_PARAMS(7, x)))
    {}

  object array_base::argmax(long axis)
  {
      return attr("argmax")(axis);
  }
  
  object array_base::argmin(long axis)
  {
      return attr("argmin")(axis);
  }
  
  object array_base::argsort(long axis)
  {
      return attr("argsort")(axis);
  }
  
  object array_base::astype(object const& type)
  {
      return attr("astype")(type);
  }
  
  void array_base::byteswap()
  {
      attr("byteswap")();
  }
  
  object array_base::copy() const
  {
      return attr("copy")();
  }
  
  object array_base::diagonal(long offset, long axis1, long axis2) const
  {
      return attr("diagonal")(offset, axis1, axis2);
  }
  
  void array_base::info() const
  {
      attr("info")();
  }
  
  bool array_base::is_c_array() const
  {
      return extract<bool>(attr("is_c_array")());
  }
  
  bool array_base::isbyteswapped() const
  {
      return extract<bool>(attr("isbyteswapped")());
  }
  
  array array_base::new_(object type) const
  {
      return extract<array>(attr("new")(type))();
  }
  
  void array_base::sort()
  {
      attr("sort")();
  }
  
  object array_base::trace(long offset, long axis1, long axis2) const
  {
      return attr("trace")(offset, axis1, axis2);
  }
  
  object array_base::type() const
  {
      return attr("type")();
  }
  
  char array_base::typecode() const
  {
      return extract<char>(attr("typecode")());
  }

  object array_base::factory(
          object const& sequence
        , object const& typecode
        , bool copy
        , bool savespace
        , object type
        , object shape
  )
  {
      return attr("factory")(sequence, typecode, copy, savespace, type, shape);
  }

  object array_base::getflat() const
  {
      return attr("getflat")();
  }
      
  long array_base::getrank() const
  {
      return extract<long>(attr("getrank")());
  }
  
  object array_base::getshape() const
  {
      return attr("getshape")();
  }
  
  bool array_base::isaligned() const
  {
      return extract<bool>(attr("isaligned")());
  }
  
  bool array_base::iscontiguous() const
  {      
      return extract<bool>(attr("iscontiguous")());
  }
  
  long array_base::itemsize() const
  {
      return extract<long>(attr("itemsize")());
  }
  
  long array_base::nelements() const
  {
      return extract<long>(attr("nelements")());
  }
  
  object array_base::nonzero() const
  {
      return attr("nonzero")();
  }
   
  void array_base::put(object const& indices, object const& values)
  {
      attr("put")(indices, values);
  }
   
  void array_base::ravel()
  {
      attr("ravel")();
  }
   
  object array_base::repeat(object const& repeats, long axis)
  {
      return attr("repeat")(repeats, axis);
  }
   
  void array_base::resize(object const& shape)
  {
      attr("resize")(shape);
  }
      
  void array_base::setflat(object const& flat)
  {
      attr("setflat")(flat);
  }
  
  void array_base::setshape(object const& shape)
  {
      attr("setshape")(shape);
  }
   
  void array_base::swapaxes(long axis1, long axis2)
  {
      attr("swapaxes")(axis1, axis2);
  }
   
  object array_base::take(object const& sequence, long axis) const
  {
      return attr("take")(sequence, axis);
  }
   
  void array_base::tofile(object const& file) const
  {
      attr("tofile")(file);
  }
   
  str array_base::tostring() const
  {
      return str(attr("tostring")());
  }
   
  void array_base::transpose(object const& axes)
  {
      attr("transpose")(axes);
  }
   
  object array_base::view() const
  {
      return attr("view")();
  }
}

}}} // namespace boost::python::numeric
