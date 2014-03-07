// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: custom.cpp 82978 2013-02-18 18:31:36Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/concept_interface.hpp>
#include <boost/type_erasure/rebind_any.hpp>
#include <vector>

namespace mpl = boost::mpl;
using namespace boost::type_erasure;

//[custom1
/*`
    Earlier, we used __BOOST_TYPE_ERASURE_MEMBER to define
    a concept for containers that support `push_back`.  Sometimes
    this interface isn't flexible enough, however.  The library
    also provides a lower level interface that gives full
    control of the behavior.  Let's take a look at what we
    would need in order to define `has_push_back.`  First,
    we need to define the `has_push_back` template itself.  We'll
    give it two template parameters, one for the container
    and one for the element type.  This template must have
    a static member function called apply which is used
    to execute the operation.
*/

template<class C, class T>
struct has_push_back
{
    static void apply(C& cont, const T& arg) { cont.push_back(arg); }
};
//]

//[custom3
/*`
    Our second task is to customize __any so that we can call `c.push_back(10)`.
    We do this by specializing __concept_interface.
    The first argument is `has_push_back`, since we want to inject a member
    into every __any that uses the `has_push_back` concept.  The second argument,
    `Base`, is used by the library to chain multiple uses of __concept_interface
    together.  We have to inherit from it publicly.  `Base` is also used
    to get access to the full __any type.  The third argument is the placeholder
    that represents this any.  If someone used `push_back<_c, _b>`,
    we only want to insert a `push_back` member in the container,
    not the value type.  Thus, the third argument is the container
    placeholder.

    When we define `push_back` the argument type uses the metafunction
    __as_param.  This is just to handle the case where `T` is a
    placeholder.  If `T` is not a placeholder, then the metafunction
    just returns its argument, `const T&`, unchanged.
*/
namespace boost {
namespace type_erasure {
template<class C, class T, class Base>
struct concept_interface<has_push_back<C, T>, Base, C> : Base
{
    void push_back(typename as_param<Base, const T&>::type arg)
    { call(has_push_back<C, T>(), *this, arg); }
};
}
}
//]

void custom2() {
    //[custom2
    /*`
        Now, we can use this in an __any using
        __call to dispatch the operation.
    */
    std::vector<int> vec;
    any<has_push_back<_self, int>, _self&> c(vec);
    int i = 10;
    call(has_push_back<_self, int>(), c, i);
    // vec is [10].
    //]
}

void custom4() {
    //[custom4
    /*`
        Our example now becomes
    */
    std::vector<int> vec;
    any<has_push_back<_self, int>, _self&> c(vec);
    c.push_back(10);
    /*`
        which is what we want.
    */
    //]
}

//[custom
//` (For the source of the examples in this section see
//` [@boost:/libs/type_erasure/example/custom.cpp custom.cpp])
//` [custom1]
//` [custom2]
//` [custom3]
//` [custom4]
//]
