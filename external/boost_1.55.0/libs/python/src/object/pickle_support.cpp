//  (C) Copyright R.W. Grosse-Kunstleve 2002.
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/make_function.hpp>
#include <boost/python/object/class.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/list.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/str.hpp>

namespace boost { namespace python {

namespace {

  tuple instance_reduce(object instance_obj)
  {
      list result;
      object instance_class(instance_obj.attr("__class__"));
      result.append(instance_class);
      object none;
      if (!getattr(instance_obj, "__safe_for_unpickling__", none))
      {
          str type_name(getattr(instance_class, "__name__"));
          str module_name(getattr(instance_class, "__module__", object("")));
          if (module_name)
              module_name += ".";

          PyErr_SetObject(
               PyExc_RuntimeError,
               ( "Pickling of \"%s\" instances is not enabled"
                 " (http://www.boost.org/libs/python/doc/v2/pickle.html)"
                  % (module_name+type_name)).ptr()
          );

          throw_error_already_set();
      }
      object getinitargs = getattr(instance_obj, "__getinitargs__", none);
      tuple initargs;
      if (!getinitargs.is_none()) {
          initargs = tuple(getinitargs());
      }
      result.append(initargs);
      object getstate = getattr(instance_obj, "__getstate__", none);
      object instance_dict = getattr(instance_obj, "__dict__", none);
      long len_instance_dict = 0;
      if (!instance_dict.is_none()) {
          len_instance_dict = len(instance_dict);
      }
      if (!getstate.is_none()) {
          if (len_instance_dict > 0) {
              object getstate_manages_dict = getattr(
                instance_obj, "__getstate_manages_dict__", none);
              if (getstate_manages_dict.is_none()) {
                  PyErr_SetString(PyExc_RuntimeError,
                    "Incomplete pickle support"
                    " (__getstate_manages_dict__ not set)");
                  throw_error_already_set();
              }
          }
          result.append(getstate());
      }
      else if (len_instance_dict > 0) {
          result.append(instance_dict);
      }
      return tuple(result);
  }

} // namespace

object const& make_instance_reduce_function()
{
    static object result(&instance_reduce);
    return result;
}

}} // namespace boost::python
