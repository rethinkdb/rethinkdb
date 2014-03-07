// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/object.hpp>
#include <boost/python/class.hpp>

using namespace boost::python;

struct X
{
    int x;
    X(int n) : x(n) { }
};

int x_function(X& x)
{   return x.x;
}


BOOST_PYTHON_MODULE(class_ext)
{
    class_<X>("X", init<int>());
    def("x_function", x_function);
}

#include "module_tail.cpp"
