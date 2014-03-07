// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/object_protocol.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/object.hpp>
#include <boost/python/ssize_t.hpp>

namespace boost { namespace python { namespace api {

BOOST_PYTHON_DECL object getattr(object const& target, object const& key)
{
    return object(detail::new_reference(PyObject_GetAttr(target.ptr(), key.ptr())));
}
    
BOOST_PYTHON_DECL object getattr(object const& target, object const& key, object const& default_)
{
    PyObject* result = PyObject_GetAttr(target.ptr(), key.ptr());
    if (result == NULL && PyErr_ExceptionMatches(PyExc_AttributeError))
    {
        PyErr_Clear();
        return default_;
    }
    return object(detail::new_reference(result));
}
        
BOOST_PYTHON_DECL void setattr(object const& target, object const& key, object const& value)
{
    if (PyObject_SetAttr(target.ptr(), key.ptr(), value.ptr()) == -1)
        throw_error_already_set();
}

BOOST_PYTHON_DECL void delattr(object const& target, object const& key)
{
    if (PyObject_DelAttr(target.ptr(), key.ptr()) == -1)
        throw_error_already_set();
}

BOOST_PYTHON_DECL object getattr(object const& target, char const* key)
{
    return object(
        detail::new_reference(
            PyObject_GetAttrString(target.ptr(), const_cast<char*>(key))
        ));
}
    
BOOST_PYTHON_DECL object getattr(object const& target, char const* key, object const& default_)
{
    PyObject* result = PyObject_GetAttrString(target.ptr(), const_cast<char*>(key));
    if (result == NULL && PyErr_ExceptionMatches(PyExc_AttributeError))
    {
        PyErr_Clear();
        return default_;
    }
    return object(detail::new_reference(result));
    
}
BOOST_PYTHON_DECL void setattr(object const& target, char const* key, object const& value)
{
    if (PyObject_SetAttrString(
            target.ptr(), const_cast<char*>(key), value.ptr()) == -1
        )
    {
        throw_error_already_set();
    }
}

BOOST_PYTHON_DECL void delattr(object const& target, char const* key)
{
    if (PyObject_DelAttrString(
            target.ptr(), const_cast<char*>(key)) == -1
        )
    {
        throw_error_already_set();
    }
}

BOOST_PYTHON_DECL object getitem(object const& target, object const& key)
{
    return object(detail::new_reference(
                      PyObject_GetItem(target.ptr(), key.ptr())));
}
    
BOOST_PYTHON_DECL void setitem(object const& target, object const& key, object const& value)
{
    if (PyObject_SetItem(target.ptr(), key.ptr(), value.ptr()) == -1)
        throw_error_already_set();
}
    
BOOST_PYTHON_DECL void delitem(object const& target, object const& key)
{
    if (PyObject_DelItem(target.ptr(), key.ptr()) == -1)
        throw_error_already_set();
}

namespace // slicing code copied directly out of the Python implementation
{
  #undef ISINT
  #define ISINT(x) ((x) == NULL || PyInt_Check(x) || PyLong_Check(x))

  static PyObject *
  apply_slice(PyObject *u, PyObject *v, PyObject *w) /* return u[v:w] */
  {
#if PY_VERSION_HEX < 0x03000000
      PyTypeObject *tp = u->ob_type;
      PySequenceMethods *sq = tp->tp_as_sequence;

      if (sq && sq->sq_slice && ISINT(v) && ISINT(w)) {
          ssize_t ilow = 0, ihigh = ssize_t_max;
          if (!_PyEval_SliceIndex(v, &ilow))
              return NULL;
          if (!_PyEval_SliceIndex(w, &ihigh))
              return NULL;
          return PySequence_GetSlice(u, ilow, ihigh);
      }
      else 
#endif
      {
          PyObject *slice = PySlice_New(v, w, NULL);
          if (slice != NULL) {
              PyObject *res = PyObject_GetItem(u, slice);
              Py_DECREF(slice);
              return res;
          }
          else
              return NULL;
      }
  }

  static int
  assign_slice(PyObject *u, PyObject *v, PyObject *w, PyObject *x)
      /* u[v:w] = x */
  {
#if PY_VERSION_HEX < 0x03000000
      PyTypeObject *tp = u->ob_type;
      PySequenceMethods *sq = tp->tp_as_sequence;

      if (sq && sq->sq_slice && ISINT(v) && ISINT(w)) {
          ssize_t ilow = 0, ihigh = ssize_t_max;
          if (!_PyEval_SliceIndex(v, &ilow))
              return -1;
          if (!_PyEval_SliceIndex(w, &ihigh))
              return -1;
          if (x == NULL)
              return PySequence_DelSlice(u, ilow, ihigh);
          else
              return PySequence_SetSlice(u, ilow, ihigh, x);
      }
      else 
#endif
      {
          PyObject *slice = PySlice_New(v, w, NULL);
          if (slice != NULL) {
              int res;
              if (x != NULL)
                  res = PyObject_SetItem(u, slice, x);
              else
                  res = PyObject_DelItem(u, slice);
              Py_DECREF(slice);
              return res;
          }
          else
              return -1;
      }
  }
}

BOOST_PYTHON_DECL object getslice(object const& target, handle<> const& begin, handle<> const& end)
{
    return object(
        detail::new_reference(
            apply_slice(target.ptr(), begin.get(), end.get())));
}

BOOST_PYTHON_DECL void setslice(object const& target, handle<> const& begin, handle<> const& end, object const& value)
{
    if (assign_slice(
            target.ptr(), begin.get(), end.get(), value.ptr()) == -1
        )
    {
        throw_error_already_set();
    }
}

BOOST_PYTHON_DECL void delslice(object const& target, handle<> const& begin, handle<> const& end)
{
    if (assign_slice(
            target.ptr(), begin.get(), end.get(), 0) == -1
        )
    {
        throw_error_already_set();
    }
}

}}} // namespace boost::python::api
