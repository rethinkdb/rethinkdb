// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/module.hpp>
#include <boost/python/class.hpp>
#include <boost/python/call_method.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/def.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include "test_class.hpp"

#include <memory>

using namespace boost::python;
using boost::shared_ptr;

class Test;
typedef shared_ptr<Test> TestPtr;

class Test : public boost::enable_shared_from_this<Test> {
public:
    static TestPtr construct() {
        return TestPtr(new Test);
    }

    void act() {
        TestPtr kungFuDeathGrip(shared_from_this());
    }

    void take(TestPtr t) {
    }
};

BOOST_PYTHON_MODULE(enable_shared_from_this_ext)
{
    class_<Test, TestPtr, boost::noncopyable>("Test")
        .def("construct", &Test::construct).staticmethod("construct")
        .def("act", &Test::act)
        .def("take", &Test::take)
        ;
}

#include "module_tail.cpp"


