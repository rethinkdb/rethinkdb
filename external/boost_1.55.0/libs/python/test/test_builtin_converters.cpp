// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <complex>
#include <boost/python/handle.hpp>
#include <boost/python/cast.hpp>
#include <boost/python/object.hpp>
#include <boost/python/detail/wrap_python.hpp>

template <class T>
struct by_value
{
    static T rewrap(T x)
    {
        return x;
    }
    static int size(void)
    {
        return sizeof(T);
    }
};

template <class T>
struct by_const_reference
{
    static T rewrap(T const& x)
    {
        return x;
    }
};

template <class T>
struct by_reference
{
    static T rewrap(T& x)
    {
        return x;
    }
};

using boost::python::def;
using boost::python::handle;
using boost::python::object;
using boost::python::borrowed;

// Used to test that arbitrary handle<>s can be returned
handle<PyTypeObject> get_type(handle<> x)
{
    return handle<PyTypeObject>(borrowed(x->ob_type));
}

handle<> return_null_handle()
{
    return handle<>();
}

char const* rewrap_value_mutable_cstring(char* x) { return x; }

object identity_(object x) { return x; }

BOOST_PYTHON_MODULE(builtin_converters_ext)
{    
    def("get_type", get_type);
    def("return_null_handle", return_null_handle);

// These methods are used solely for getting some C++ type sizes
    def("bool_size", by_value<bool>::size);
    def("char_size", by_value<char>::size);
    def("int_size", by_value<int>::size);
    def("short_size", by_value<short>::size);
    def("long_size", by_value<long>::size);
#ifdef HAVE_LONG_LONG
    def("long_long_size", by_value<BOOST_PYTHON_LONG_LONG>::size);
#endif

    def("rewrap_value_bool", by_value<bool>::rewrap);
    def("rewrap_value_char", by_value<char>::rewrap);
    def("rewrap_value_signed_char", by_value<signed char>::rewrap);
    def("rewrap_value_unsigned_char", by_value<unsigned char>::rewrap);
    def("rewrap_value_int", by_value<int>::rewrap);
    def("rewrap_value_unsigned_int", by_value<unsigned int>::rewrap);
    def("rewrap_value_short", by_value<short>::rewrap);
    def("rewrap_value_unsigned_short", by_value<unsigned short>::rewrap);
    def("rewrap_value_long", by_value<long>::rewrap);
    def("rewrap_value_unsigned_long", by_value<unsigned long>::rewrap);
// using Python's macro instead of Boost's - we don't seem to get the
// config right all the time.
#ifdef HAVE_LONG_LONG
    def("rewrap_value_long_long", by_value<BOOST_PYTHON_LONG_LONG>::rewrap);
    def("rewrap_value_unsigned_long_long", by_value<unsigned BOOST_PYTHON_LONG_LONG>::rewrap);
# endif 
    def("rewrap_value_float", by_value<float>::rewrap);
    def("rewrap_value_double", by_value<double>::rewrap);
    def("rewrap_value_long_double", by_value<long double>::rewrap);
    def("rewrap_value_complex_float", by_value<std::complex<float> >::rewrap);
    def("rewrap_value_complex_double", by_value<std::complex<double> >::rewrap);
    def("rewrap_value_complex_long_double", by_value<std::complex<long double> >::rewrap);
    def("rewrap_value_wstring",
# if defined(BOOST_NO_STD_WSTRING) || !defined(Py_USING_UNICODE)
        identity_
# else 
        by_value<std::wstring>::rewrap
# endif 
    );
    def("rewrap_value_string",
# if defined(BOOST_NO_STD_WSTRING) || !defined(Py_USING_UNICODE)
        identity_
# else 
        by_value<std::wstring>::rewrap
# endif 
    );
    def("rewrap_value_string", by_value<std::string>::rewrap);
    def("rewrap_value_cstring", by_value<char const*>::rewrap);
    def("rewrap_value_handle", by_value<handle<> >::rewrap);
    def("rewrap_value_object", by_value<object>::rewrap);

        // Expose this to illustrate our failings ;-). See test_builtin_converters.py
    def("rewrap_value_mutable_cstring", rewrap_value_mutable_cstring);


    def("rewrap_const_reference_bool", by_const_reference<bool>::rewrap);
    def("rewrap_const_reference_char", by_const_reference<char>::rewrap);
    def("rewrap_const_reference_signed_char", by_const_reference<signed char>::rewrap);
    def("rewrap_const_reference_unsigned_char", by_const_reference<unsigned char>::rewrap);
    def("rewrap_const_reference_int", by_const_reference<int>::rewrap);
    def("rewrap_const_reference_unsigned_int", by_const_reference<unsigned int>::rewrap);
    def("rewrap_const_reference_short", by_const_reference<short>::rewrap);
    def("rewrap_const_reference_unsigned_short", by_const_reference<unsigned short>::rewrap);
    def("rewrap_const_reference_long", by_const_reference<long>::rewrap);
    def("rewrap_const_reference_unsigned_long", by_const_reference<unsigned long>::rewrap);
// using Python's macro instead of Boost's - we don't seem to get the
// config right all the time.
# ifdef HAVE_LONG_LONG
    def("rewrap_const_reference_long_long", by_const_reference<BOOST_PYTHON_LONG_LONG>::rewrap);
    def("rewrap_const_reference_unsigned_long_long", by_const_reference<unsigned BOOST_PYTHON_LONG_LONG>::rewrap);
# endif
    def("rewrap_const_reference_float", by_const_reference<float>::rewrap);
    def("rewrap_const_reference_double", by_const_reference<double>::rewrap);
    def("rewrap_const_reference_long_double", by_const_reference<long double>::rewrap);
    def("rewrap_const_reference_complex_float", by_const_reference<std::complex<float> >::rewrap);
    def("rewrap_const_reference_complex_double", by_const_reference<std::complex<double> >::rewrap);
    def("rewrap_const_reference_complex_long_double", by_const_reference<std::complex<long double> >::rewrap);
    def("rewrap_const_reference_string", by_const_reference<std::string>::rewrap);
    def("rewrap_const_reference_cstring", by_const_reference<char const*>::rewrap);
    def("rewrap_const_reference_handle", by_const_reference<handle<> >::rewrap);
    def("rewrap_const_reference_object", by_const_reference<object>::rewrap);
    def("rewrap_reference_object", by_reference<object>::rewrap);
}

