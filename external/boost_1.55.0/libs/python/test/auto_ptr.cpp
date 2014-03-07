// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/module.hpp>
#include "test_class.hpp"
#include <boost/python/class.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/def.hpp>
#include <boost/python/implicit.hpp>

#include <boost/detail/workaround.hpp>

#include <memory>

using namespace boost::python;

typedef test_class<> X;

struct Y : X
{
    Y(int n) : X(n) {};
};

int look(std::auto_ptr<X> const& x)
{
    return (x.get()) ? x->value() : -1;
}

int steal(std::auto_ptr<X> x)
{
    return x->value();
}

int maybe_steal(std::auto_ptr<X>& x, bool doit)
{
    int n = x->value();
    if (doit)
        x.release();
    return n;
}

std::auto_ptr<X> make()
{
    return std::auto_ptr<X>(new X(77));
}

std::auto_ptr<X> callback(object f)
{
    std::auto_ptr<X> x(new X(77));
    return call<std::auto_ptr<X> >(f.ptr(), x);
}

std::auto_ptr<X> extract_(object o)
{
    return extract<std::auto_ptr<X>&>(o)
#if BOOST_MSVC <= 1300
        ()
#endif 
        ;
}

BOOST_PYTHON_MODULE(auto_ptr_ext)
{
    class_<X, std::auto_ptr<X>, boost::noncopyable>("X", init<int>())
        .def("value", &X::value)
        ;
    
    class_<Y, std::auto_ptr<Y>, bases<X>, boost::noncopyable>("Y", init<int>())
        ;

    // VC6 auto_ptrs do not have converting constructors    
#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, < 306)
    scope().attr("broken_auto_ptr") = 1;
#else
    scope().attr("broken_auto_ptr") = 0;
    implicitly_convertible<std::auto_ptr<Y>, std::auto_ptr<X> >();
#endif
    
    def("look", look);
    def("steal", steal);
    def("maybe_steal", maybe_steal);
    def("make", make);
    def("callback", callback);
    def("extract", extract_);
}

#include "module_tail.cpp"

