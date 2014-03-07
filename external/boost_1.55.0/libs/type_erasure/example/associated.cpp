// Boost.TypeErasure library
//
// Copyright 2012 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: associated.cpp 79777 2012-07-28 03:43:26Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/deduced.hpp>
#include <boost/type_erasure/same_type.hpp>
#include <boost/pointee.hpp>
#include <boost/mpl/vector.hpp>
#include <iostream>

namespace mpl = boost::mpl;
using namespace boost::type_erasure;

//[associated1
/*`
    Associated types are defined using the __deduced
    template.  __deduced is just like an ordinary
    placeholder, except that the type that it binds
    to is determined by calling a metafunction and
    does not need to be specified explicitly.

    For example, we can define a concept for
    holding any iterator, raw pointer, or
    smart pointer as follows.

    Note the extra trickery to make sure that it is safe
    to instantiate pointee with a placeholder, because
    argument dependant lookup can cause spurious instantiations.
*/

template<class T>
struct pointee
{
    typedef typename mpl::eval_if<is_placeholder<T>,
        mpl::identity<void>,
        boost::pointee<T>
    >::type type;
};

template<class T = _self>
struct pointer :
    mpl::vector<
        copy_constructible<T>,
        dereferenceable<deduced<pointee<T> >&, T>
    >
{
    // provide a typedef for convenience
    typedef deduced<pointee<T> > element_type;
};

//]

void associated2() {
    //[associated2
    /*`
        Now the Concept of `x` uses two placeholders, `_self`
        and `pointer<>::element_type`.  When we construct `x`,
        with an `int*`, `pointer<>::element_type` is deduced
        as `pointee<int*>::type` which is `int`.  Thus, dereferencing
        `x` returns an __any that contains an `int`.
    */
    int i = 10;
    any<
        mpl::vector<
            pointer<>,
            typeid_<pointer<>::element_type>
        >
    > x(&i);
    int j = any_cast<int>(*x); // j == i
    //]
}

void associated3() {
    //[associated3
    /*`
        Sometimes we want to require that the associated
        type be a specific type.  This can be solved using
        the __same_type concept.  Here we create an any that
        can hold any pointer whose element type is `int`.
    */
    int i = 10;
    any<
        mpl::vector<
            pointer<>,
            same_type<pointer<>::element_type, int>
        >
    > x(&i);
    std::cout << *x << std::endl; // prints 10
    /*`
        Using __same_type like this effectively causes the library to
        replace all uses of `pointer<>::element_type` with `int`
        and validate that it is always bound to `int`.
        Thus, dereferencing `x` now returns an `int`.
    */
    //]
}

void associated4() {
    //[associated4
    /*`
        __same_type can also be used for two placeholders.
        This allows us to use a simple name instead of
        writing out an associated type over and over.
    */
    int i = 10;
    any<
        mpl::vector<
            pointer<>,
            same_type<pointer<>::element_type, _a>,
            typeid_<_a>,
            copy_constructible<_a>,
            addable<_a>,
            ostreamable<std::ostream, _a>
        >
    > x(&i);
    std::cout << (*x + *x) << std::endl; // prints 20
    //]
}

//[associated
//` (For the source of the examples in this section see
//` [@boost:/libs/type_erasure/example/associated.cpp associated.cpp])
//` [associated1]
//` [associated2]
//` [associated3]
//` [associated4]
//]
