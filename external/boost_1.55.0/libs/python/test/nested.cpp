// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/module.hpp>
#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/scope.hpp>
#include "test_class.hpp"
#if __GNUC__ != 2
# include <ostream>
#else
# include <ostream.h>
#endif

typedef test_class<> X;
typedef test_class<1> Y;

std::ostream& operator<<(std::ostream& s, X const& x)
{
    return s << x.value();
}

std::ostream& operator<<(std::ostream& s, Y const& x)
{
    return s << x.value();
}


BOOST_PYTHON_MODULE(nested_ext)
{
    using namespace boost::python;

    // Establish X as the current scope.
    scope x_class
        = class_<X>("X", init<int>())
          .def(str(self))
        ;


    // Y will now be defined in the current scope
    class_<Y>("Y", init<int>())
        .def(str(self))
        ;
}


#include "module_tail.cpp"



