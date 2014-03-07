// Copyright Eric Niebler 2005.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Credits:
//   Andreas Kl\:ockner for fixing increment() to handle
//   error conditions.

#include <boost/python/object.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/object/stl_iterator_core.hpp>

namespace boost { namespace python { namespace objects
{ 

stl_input_iterator_impl::stl_input_iterator_impl()
  : it_()
  , ob_()
{
}

stl_input_iterator_impl::stl_input_iterator_impl(boost::python::object const &ob)
  : it_(ob.attr("__iter__")())
  , ob_()
{
    this->increment();
}

void stl_input_iterator_impl::increment()
{
    this->ob_ = boost::python::handle<>(
        boost::python::allow_null(PyIter_Next(this->it_.ptr())));
    if (PyErr_Occurred())
        throw boost::python::error_already_set();
}

bool stl_input_iterator_impl::equal(stl_input_iterator_impl const &that) const
{
    return !this->ob_ == !that.ob_;
}

boost::python::handle<> const &stl_input_iterator_impl::current() const
{
    return this->ob_;
}

}}} // namespace boost::python::objects
