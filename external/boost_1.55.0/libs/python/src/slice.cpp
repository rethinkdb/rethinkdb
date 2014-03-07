#include "boost/python/slice.hpp"

// Copyright (c) 2004 Jonathan Brandmeyer
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


namespace boost { namespace python { namespace detail {

slice_base::slice_base(PyObject* start, PyObject* stop, PyObject* step)
  : object(detail::new_reference( PySlice_New(start, stop, step)))
{
}

object
slice_base::start() const
{
    return object( detail::borrowed_reference(
        ((PySliceObject*)this->ptr())->start));
}

object
slice_base::stop() const
{
    return object( detail::borrowed_reference(
        ((PySliceObject*)this->ptr())->stop));
}

object
slice_base::step() const
{
    return object( detail::borrowed_reference(
        ((PySliceObject*)this->ptr())->step));
}

} } } // !namespace boost::python::detail
