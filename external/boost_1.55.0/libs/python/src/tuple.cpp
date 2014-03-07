// Copyright David Abrahams 2004.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/tuple.hpp>

namespace boost { namespace python { namespace detail {

detail::new_reference tuple_base::call(object const& arg_)
{
    return (detail::new_reference)PyObject_CallFunction(
        (PyObject*)&PyTuple_Type, const_cast<char*>("(O)"), 
        arg_.ptr());
}
    
tuple_base::tuple_base()
    : object(detail::new_reference(PyTuple_New(0)))
{}
    
tuple_base::tuple_base(object_cref sequence)
    : object(call(sequence))
{}

static struct register_tuple_pytype_ptr
{
    register_tuple_pytype_ptr()
    {
        const_cast<converter::registration &>(
            converter::registry::lookup(boost::python::type_id<boost::python::tuple>())
            ).m_class_object = &PyTuple_Type;
    }
}register_tuple_pytype_ptr_;


}}}  // namespace boost::python
