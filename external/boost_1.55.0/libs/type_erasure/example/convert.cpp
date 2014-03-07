// Boost.TypeErasure library
//
// Copyright 2012 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: convert.cpp 79728 2012-07-24 22:07:53Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>

namespace mpl = boost::mpl;
using namespace boost::type_erasure;

void convert1() {
//[convert1
    /*`
        An __any can be converted to another __any
        as long as the conversion is an "upcast."
    */

    typedef any<
        mpl::vector<
            copy_constructible<>,
            typeid_<>,
            ostreamable<>
        >
    > any_printable;
    typedef any<
        mpl::vector<
            copy_constructible<>,
            typeid_<>
        >
    > common_any;
    any_printable x(10);
    common_any y(x);

    /*`
        This conversion is okay because the requirements of `common_any`
        are a subset of the requirements of `any_printable`.  Conversion
        in the other direction is illegal.

        ``
            common_any x(10);
            any_printable y(x); // error
        ``
    */
//]
}

//[convert
//` (For the source of the examples in this section see
//` [@boost:/libs/type_erasure/example/convert.cpp convert.cpp])
//` [convert1]
//]
