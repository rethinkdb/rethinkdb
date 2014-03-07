
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/config

// MACRO:       BOOST_NO_CXX11_LOCAL_CLASS_TEMPLATE_PARAMETERS
// TITLE:       local classes as template parameters
// DESCRIPTION: Local classes cannot be passed as template parameters.

// NOTE: Local classes cannot be passed as template parameters in C++03 (even
// if some C++03 compilers, like MSVC and older GCC, allow it). Local classes
// can instead be passed as template parameters in C++11 (see also N2657, note
// that this macro does not check if unnamed types can also be passed as
// template parameters but it is intentionally limited to local named classes
// because some non C++11 compilers might only support local named classes as
// template parameters which is still very useful to program local functors).
namespace boost_no_cxx11_local_class_template_parameters {

template<typename T> struct a { void use() {} };
template<typename T> void f(T) {}

int test() {
    class local_class {} local_obj;
    a<local_class> a1;
    a1.use(); // Avoid unused variable warning.
    f(local_obj);
    return 0;
}

} // namespace

