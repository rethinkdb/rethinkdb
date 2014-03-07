// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: references.cpp 83251 2013-03-02 19:23:44Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>

namespace mpl = boost::mpl;
using namespace boost::type_erasure;

void references1() {
    //[references1
    /*`
        To capture by reference, we simply add a reference
        to the __placeholder.
    */
    int i;
    any<typeid_<>, _self&> x(i);
    any_cast<int&>(x) = 5; // now i is 5
    /*`
        [note `_self` is the default __placeholder, so it is
        easiest to use `_self&`.  We could use another
        __placeholder instead. __any`<`__typeid_`<_a>, _a&>` has
        exactly the same behavior.]
    */
    //]
}

void references2() {
    //[references2
    /*`
        References cannot be rebound.  Just like a built-in C++ reference,
        once you've initialized it you can't change it to point to
        something else.
        ``
            int i, j;
            any<typeid_<>, _self&> x(i), y(j);
            x = y; // error
        ``

        [note As with any other operation, `x = y` for references
        acts on `i` and `j`.  Assignment like this is legal
        if __assignable`<>` is in the Concept, but `x` would
        still hold a reference to `i`.]
    */
    //]
}

void references3() {
    //[references3
    /*`
        A reference can be bound to another __any.
    */
    typedef mpl::vector<
        copy_constructible<>,
        incrementable<>
    > requirements;

    any<requirements> x(10);
    any<requirements, _self&> y(x);
    ++y; // x is now 11
    //]
}

void references4() {
    //[references4
    /*`
        If a reference is used after the underlying object
        goes out of scope or is reset, the behavior is undefined.
    */
    typedef mpl::vector<
        copy_constructible<>,
        incrementable<>,
        relaxed
    > requirements;
    any<requirements> x(10);
    any<requirements, _self&> y(x);
    x = 1.0;
    ++y; // undefined behavior.
    //]
}

void references5() {
    typedef mpl::vector<
        copy_constructible<>,
        incrementable<>
    > requirements;
    //[references5
    /*`
        This only applies when a reference is constructed
        from a value.  If a reference is constructed from another
        reference, the new reference does not depend on the old one.
    */
    any<requirements> x(10);
    boost::shared_ptr<any<requirements, _self&> > p(
        new any<requirements, _self&>(x));
    any<requirements, _self&> y(*p); // equivalent to y(x);
    p.reset();
    ++y; // okay
    //]
}

void references6() {
    //[references6
    /*`
        Both const and non-const references are supported.
     */
    int i = 0;
    any<incrementable<>, _self&> x(i);
    any<incrementable<>, const _self&> y(x);
    /*`
        A reference to non-const can be converted to a reference
        to const, but not the other way around.  Naturally,
        we can't apply mutating operations to a const reference.

            any<incrementable<>, _self&> z(y); // error
            ++y; // error
     */
    //]
}

//[references
//` (For the source of the examples in this section see
//` [@boost:/libs/type_erasure/example/references.cpp references.cpp])
//` [references1]
//` [references2]
//` [references3]
//` [references4]
//` [references5]
//` [references6]
//]
