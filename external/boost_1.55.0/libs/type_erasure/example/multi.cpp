// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: multi.cpp 83393 2013-03-10 03:48:33Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/tuple.hpp>
#include <boost/mpl/vector.hpp>
#include <iostream>

namespace mpl = boost::mpl;
using namespace boost::type_erasure;

void multi1() {
    //[multi1
    /*`
        Operations can have more than one __any argument.
        Let's use binary addition as an example.
    */
    typedef any<
        mpl::vector<
            copy_constructible<>,
            typeid_<>,
            addable<>,
            ostreamable<>
        >
    > any_type;
    any_type x(10);
    any_type y(7);
    any_type z(x + y);
    std::cout << z << std::endl; // prints 17
    /*`
        This is /not/ a multimethod.  The underlying types of the
        arguments of `+` must be the same or the behavior is undefined.
        This example is correct because the arguments both hold
        `int`'s.

        [note Adding __relaxed leads an exception rather than undefined
        behavior if the argument types are wrong.]
    */
    //]
}

void multi2() {
    //[multi2
    /*`
        __addable`<>` requires the types of the arguments to be exactly
        the same.  This doesn't cover all uses of addition though.  For
        example, pointer arithmetic takes a pointer and an integer and
        returns a pointer.  We can capture this kind of relationship among
        several types by identifying each type involved with a placeholder.
        We'll let the placeholder `_a` represent the pointer and the
        placeholder `_b` represent the integer.
    */

    int array[5];

    typedef mpl::vector<
        copy_constructible<_a>,
        copy_constructible<_b>,
        typeid_<_a>,
        addable<_a, _b, _a>
    > requirements;

    /*`
        Our new concept, `addable<_a, _b, _a>` captures the
        rules of pointer addition: `_a + _b -> _a`.

        Also, we can no longer capture the variables
        independently.
        ``
            any<requirements, _a> ptr(&array[0]); // illegal
        ``
        This doesn't work because the library needs
        to know the type that _b binds to when it
        captures the concept bindings.  We need to
        specify the bindings of both placeholders
        when we construct the __any.
     */

    typedef mpl::map<mpl::pair<_a, int*>, mpl::pair<_b, int> > types; 
    any<requirements, _a> ptr(&array[0], make_binding<types>());
    any<requirements, _b> idx(2, make_binding<types>());
    any<requirements, _a> x(ptr + idx);
    // x now holds array + 2

    /*`
        Now that the arguments of `+` aren't the same type,
        we require that both arguments agree that `_a` maps
        to `int*` and that `_b` maps to `int`.

        We can also use __tuple to avoid having to
        write out the map out explicitly.  __tuple is
        just a convenience class that combines the
        placeholder bindings it gets from all its arguments.
     */
    tuple<requirements, _a, _b> t(&array[0], 2);
    any<requirements, _a> y(get<0>(t) + get<1>(t));
    //]
}


//[multi
//` (For the source of the examples in this section see
//` [@boost:/libs/type_erasure/example/multi.cpp multi.cpp])
//` [multi1]
//` [multi2]
//]
