// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/class.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include "test_class.hpp"

using namespace boost::python;

typedef test_class<> X;

int x_value(X const& x)
{
    return x.value();
}

X make_x(int n) { return X(n); }


// foo/bar -- a regression for a vc7 bug workaround
struct bar {};
struct foo
{
    virtual ~foo() {}; // silence compiler warnings
    virtual void f() = 0;
    operator bar() const { return bar(); }
};

BOOST_PYTHON_MODULE(implicit_ext)
{
    implicitly_convertible<foo,bar>();
    implicitly_convertible<int,X>();
    
    def("x_value", x_value);
    def("make_x", make_x);

    class_<X>("X", init<int>())
        .def("value", &X::value)
        .def("set", &X::set)
        ;
    
    implicitly_convertible<X,int>();
}

#include "module_tail.cpp"
