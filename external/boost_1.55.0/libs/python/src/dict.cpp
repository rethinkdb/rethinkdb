// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/dict.hpp>
#include <boost/python/extract.hpp>

namespace boost { namespace python { namespace detail {
namespace
{
  // When returning list objects from methods, it may turn out that the
  // derived class is returning something else, perhaps something not
  // even derived from list. Since it is generally harmless for a
  // Boost.Python wrapper object to hold an object of a different
  // type, and because calling list() with an object may in fact
  // perform a conversion, the least-bad alternative is to assume that
  // we have a Python list object and stuff it into the list result.
  list assume_list(object const& o)
  {
      return list(detail::borrowed_reference(o.ptr()));
  }

  // No PyDict_CheckExact; roll our own.
  inline bool check_exact(dict_base const* p)
  {
      return  p->ptr()->ob_type == &PyDict_Type;
  }
}

detail::new_reference dict_base::call(object const& arg_)
{
    return (detail::new_reference)PyObject_CallFunction(
        (PyObject*)&PyDict_Type, const_cast<char*>("(O)"), 
        arg_.ptr());
}

dict_base::dict_base()
    : object(detail::new_reference(PyDict_New()))
{}
    
dict_base::dict_base(object_cref data)
    : object(call(data))
{}
    
void dict_base::clear()
{
    if (check_exact(this))
        PyDict_Clear(this->ptr());
    else
        this->attr("clear")();
}

dict dict_base::copy()
{
    if (check_exact(this))
    {
        return dict(detail::new_reference(
                        PyDict_Copy(this->ptr())));
    }
    else
    {
        return dict(detail::borrowed_reference(
                        this->attr("copy")().ptr()
                        ));
    }
}

object dict_base::get(object_cref k) const
{
    if (check_exact(this))
    {
        PyObject* result = PyDict_GetItem(this->ptr(),k.ptr());
        return object(detail::borrowed_reference(result ? result : Py_None));
    }
    else
    {
        return this->attr("get")(k);
    }
}

object dict_base::get(object_cref k, object_cref d) const
{
    return this->attr("get")(k,d);
}

bool dict_base::has_key(object_cref k) const
{
    return extract<bool>(this->contains(k)); 
}

list dict_base::items() const
{
    if (check_exact(this))
    {
        return list(detail::new_reference(
                        PyDict_Items(this->ptr())));
    }
    else
    {
        return assume_list(this->attr("items")());
    }
}

object dict_base::iteritems() const
{
    return this->attr("iteritems")();
}

object dict_base::iterkeys() const
{
    return this->attr("iterkeys")();
}

object dict_base::itervalues() const
{
    return this->attr("itervalues")();
}

list dict_base::keys() const
{
    if (check_exact(this))
    {
        return list(detail::new_reference(
                        PyDict_Keys(this->ptr())));
    }
    else
    {
        return assume_list(this->attr("keys")());
    }
}

tuple dict_base::popitem()
{
    return tuple(detail::borrowed_reference(
                     this->attr("popitem")().ptr()
                     ));
}

object dict_base::setdefault(object_cref k)
{
    return this->attr("setdefault")(k);
}

object dict_base::setdefault(object_cref k, object_cref d)
{
    return this->attr("setdefault")(k,d);
}

void dict_base::update(object_cref other)
{
    if (check_exact(this))
    {
        if (PyDict_Update(this->ptr(),other.ptr()) == -1)
            throw_error_already_set();
    }
    else
    {
        this->attr("update")(other);
    }
}

list dict_base::values() const
{
    if (check_exact(this))
    {
        return list(detail::new_reference(
                        PyDict_Values(this->ptr())));
    }
    else
    {
        return assume_list(this->attr("values")());
    }
}

static struct register_dict_pytype_ptr
{
    register_dict_pytype_ptr()
    {
        const_cast<converter::registration &>(
            converter::registry::lookup(boost::python::type_id<boost::python::dict>())
            ).m_class_object = &PyDict_Type;
    }
}register_dict_pytype_ptr_;

}}}  // namespace boost::python
