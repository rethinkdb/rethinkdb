// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/converter/arg_to_python.hpp>

struct X : PyObject {};

int main()
{
    boost::python::converter::arg_to_python<X*> x(0);
    return 0;
}
