// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: construction.cpp 83371 2013-03-09 19:03:08Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/tuple.hpp>
#include <boost/type_erasure/binding_of.hpp>
#include <string>
#include <vector>

namespace mpl = boost::mpl;
using namespace boost::type_erasure;

void construction1() {
    //[construction1
    /*`
        The library provides the __constructible concept to
        allow an __any to capture constructors.  The single
        template argument should be a function signature.
        The return type must be a placeholder specifying
        the type to be constructed.  The arguments are
        the arguments of the constructor.
    */
    typedef mpl::vector<
        copy_constructible<_a>,
        copy_constructible<_b>,
        copy_constructible<_c>,
        constructible<_a(const _b&, const _c&)>
    > construct;

    typedef mpl::map<
        mpl::pair<_a, std::vector<double> >,
        mpl::pair<_b, std::size_t>,
        mpl::pair<_c, double>
    > types;

    any<construct, _b> size(std::size_t(10), make_binding<types>());
    any<construct, _c> val(2.5, make_binding<types>());
    any<construct, _a> v(size, val);
    // v holds std::vector<double>(10, 2.5);
    //]
}

void construction3() {
    //[construction3
    /*`
        Now, suppose that we want a default constructor?
        We can't have the default constructor of __any
        call the default constructor of the contained type,
        because it would have no way of knowing what the
        contained type is.  So, we'll need to pass
        the placeholder binding information explicitly.
    */
    typedef mpl::vector<
        copy_constructible<>,
        constructible<_self()>
    > construct;

    any<construct> x(std::string("Test"));
    any<construct> y(binding_of(x)); // y == ""
    //]
}

void construction4() {
    //[construction4
    /*`
        This method is not restricted to the default constructor.  If
        the constructor takes arguments, they can be passed after the
        bindings.
    */
    typedef mpl::vector<
        copy_constructible<>,
        constructible<_self(std::size_t, char)>
    > construct;

    any<construct> x(std::string("Test"));
    any<construct> y(binding_of(x), 5, 'A');
    //]
}

//[construction
//` (For the source of the examples in this section see
//` [@boost:/libs/type_erasure/example/construction.cpp construction.cpp])
//` [construction1]
//` [construction3]
//` [construction4]
//]
