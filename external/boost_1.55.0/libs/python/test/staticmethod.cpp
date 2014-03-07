// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/class.hpp>
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/call_method.hpp>
#include <boost/ref.hpp>
#include <boost/utility.hpp>
#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>

using namespace boost::python;

struct X
{
    explicit X(int x) : x(x), magic(7654321) { ++counter; }
    X(X const& rhs) : x(rhs.x), magic(7654321) { ++counter; }
    virtual ~X() { BOOST_ASSERT(magic == 7654321); magic = 6666666; x = 9999; --counter; }

    void set(int _x) { BOOST_ASSERT(magic == 7654321); this->x = _x; }
    int value() const { BOOST_ASSERT(magic == 7654321); return x; }
    static int count() { return counter; }
 private:
    void operator=(X const&);
 private:
    int x;
    long magic;
    static int counter;
};
int X::counter;
int getXmagic(){return 7654321;}

BOOST_PYTHON_MODULE(staticmethod_ext)
{
    class_<X>("X", init<int>())
        .def("value", &X::value)
        .def("set", &X::set)
        .def("count", &X::count)
        .staticmethod("count")
        .def("magic", &getXmagic)
        .staticmethod("magic")
        ;
}

#include "module_tail.cpp"
