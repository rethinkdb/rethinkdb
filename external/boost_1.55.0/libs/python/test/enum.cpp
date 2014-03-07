// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/enum.hpp>
#include <boost/python/def.hpp>
#include <boost/python/module.hpp>
#include <boost/python/class.hpp>
#if BOOST_WORKAROUND(__MWERKS__, <= 0x2407)
# include <boost/type_traits/is_enum.hpp>
# include <boost/mpl/bool.hpp>
#endif 
using namespace boost::python;

enum color { red = 1, green = 2, blue = 4, blood = 1 };

#if BOOST_WORKAROUND(__MWERKS__, <= 0x2407)
namespace boost  // Pro7 has a hard time detecting enums
{
  template <> struct is_enum<color> : boost::mpl::true_ {};
}
#endif 

color identity_(color x) { return x; }

struct colorized {
    colorized() : x(red) {}
    color x;
};

BOOST_PYTHON_MODULE(enum_ext)
{
    enum_<color>("color")
        .value("red", red)
        .value("green", green)
        .value("blue", blue)
        .value("blood", blood)
        .export_values()
        ;
    
    def("identity", identity_);

#if BOOST_WORKAROUND(__MWERKS__, <=0x2407)
    color colorized::*px = &colorized::x;
    class_<colorized>("colorized")
        .def_readwrite("x", px)
        ;
#else 
    class_<colorized>("colorized")
        .def_readwrite("x", &colorized::x)
        ;
#endif 
}

#include "module_tail.cpp"
