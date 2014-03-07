// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/long.hpp>
#include <boost/python/class.hpp>
#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>

using namespace boost::python;

object new_long()
{
    return long_();
}

long_ longify(object x)
{
    return long_(x);
}

object longify_string(char const* s)
{
    return long_(s);
}

char const* is_long1(long_& x)
{
    long_ y = x;
    x += 50;
    BOOST_ASSERT(x == y + 50);
    return "yes";
}

int is_long2(char const*)
{
    return 0;
}

// tests for accepting objects (and derived classes) in constructors
// from "Milind Patil" <milind_patil-at-hotmail.com>

struct Y
{
    Y(boost::python::long_) {}
};

BOOST_PYTHON_MODULE(long_ext)
{
    def("new_long", new_long);
    def("longify", longify);
    def("longify_string", longify_string);
    def("is_long", is_long1);
    def("is_long", is_long2);
    
    class_< Y >("Y", init< boost::python::long_ >())
        ;
}

#include "module_tail.cpp"
