// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/detail/wrap_python.hpp>
#include <boost/python/borrowed.hpp>
#include <boost/static_assert.hpp>

using namespace boost::python;

template <class T>
void assert_borrowed_ptr(T const&)
{
    BOOST_STATIC_ASSERT(boost::python::detail::is_borrowed_ptr<T>::value);
}
    
template <class T>
void assert_not_borrowed_ptr(T const&)
{
    BOOST_STATIC_ASSERT(!boost::python::detail::is_borrowed_ptr<T>::value);
}
    
int main()
{
    assert_borrowed_ptr(borrowed((PyObject*)0));
    assert_borrowed_ptr(borrowed((PyTypeObject*)0));
    assert_borrowed_ptr((detail::borrowed<PyObject> const*)0);
    assert_borrowed_ptr((detail::borrowed<PyObject> volatile*)0);
    assert_borrowed_ptr((detail::borrowed<PyObject> const volatile*)0);
    assert_not_borrowed_ptr((PyObject*)0);
    assert_not_borrowed_ptr(0);
    return 0;
}
