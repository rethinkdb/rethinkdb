// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: basic.cpp 83373 2013-03-09 19:15:15Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/member.hpp>
#include <boost/type_erasure/free.hpp>
#include <boost/mpl/vector.hpp>
#include <iostream>
#include <vector>

namespace mpl = boost::mpl;
using namespace boost::type_erasure;

void basic1() {
    //[basic1
    /*`
        The main class in the library is __any.  An __any can
        store objects that meet whatever requirements we specify.
        These requirements are passed to __any as an MPL sequence.

        [note The MPL sequence combines multiple concepts.
        In the rare case when we only want a single concept, it doesn't
        need to be wrapped in an MPL sequence.]
    */
    any<mpl::vector<copy_constructible<>, typeid_<>, relaxed> > x(10);
    int i = any_cast<int>(x); // i == 10
    /*`
        __copy_constructible is a builtin concept that allows us to
        copy and destroy the object.  __typeid_ provides run-time
        type information so that we can use __any_cast.  __relaxed
        enables various useful defaults.  Without __relaxed,
        __any supports /exactly/ what you specify and nothing else.
        In particular, it allows default construction and assignment of __any.
     */
    //]
}

void basic2() {
    //[basic2
    /*`
        Now, this example doesn't do very much.  `x` is approximately
        equivalent to a [@boost:/libs/any/index.html boost::any].
        We can make it more interesting by adding some operators,
        such as `operator++` and `operator<<`.
    */
    any<
        mpl::vector<
            copy_constructible<>,
            typeid_<>,
            incrementable<>,
            ostreamable<>
        >
    > x(10);
    ++x;
    std::cout << x << std::endl; // prints 11
    //]
}

//[basic3
/*`
    The library provides concepts for most C++ operators, but this
    obviously won't cover all use cases;  we often need to
    define our own requirements.  Let's take the `push_back`
    member, defined by several STL containers.
*/

BOOST_TYPE_ERASURE_MEMBER((has_push_back), push_back, 1)

void append_many(any<has_push_back<void(int)>, _self&> container) {
    for(int i = 0; i < 10; ++i)
        container.push_back(i);
}

/*`
    We use the macro __BOOST_TYPE_ERASURE_MEMBER 
    to define a concept called `has_push_back`.
    The second parameter is the name of the member
    function and the last macro parameter indicates
    the number of arguments which is `1` since `push_back`
    is unary.  When we use `has_push_back`, we have to
    tell it the signature of the function, `void(int)`.
    This means that the type we store in the any
    has to have a member that looks like:

    ``
    void push_back(int);
    ``

    Thus, we could call `append_many` with `std::vector<int>`,
    `std::list<int>`, or `std::vector<long>` (because `int` is
    convertible to `long`), but not `std::list<std::string>`
    or `std::set<int>`.

    Also, note that `append_many` has to operate directly
    on its argument.  It cannot make a copy.  To handle this
    we use `_self&` as the second argument of __any.  `_self`
    is a __placeholder.  By using `_self&`, we indicate that
    the __any stores a reference to an external object instead of
    allocating its own object.
*/

/*`
    There's actually another __placeholder here.  The second
    parameter of `has_push_back` defaults to `_self`.  If
    we wanted to define a const member function, we would
    have to change it to `const _self`, as shown below.
 */
BOOST_TYPE_ERASURE_MEMBER((has_empty), empty, 0)
bool is_empty(any<has_empty<bool(), const _self>, const _self&> x) {
    return x.empty();
}

/*`
    For free functions, we can use the macro __BOOST_TYPE_ERASURE_FREE.
*/

BOOST_TYPE_ERASURE_FREE((has_getline), getline, 2)
std::vector<std::string> read_lines(any<has_getline<bool(_self&, std::string&)>, _self&> stream)
{
    std::vector<std::string> result;
    std::string tmp;
    while(getline(stream, tmp))
        result.push_back(tmp);
    return result;
}

/*`
    The use of `has_getline` is very similar to `has_push_back` above.
    The difference is that the placeholder `_self` is passed in
    the function signature instead of as a separate argument.

    The __placeholder doesn't have to be the first argument.
    We could just as easily make it the second argument.
*/


void read_line(any<has_getline<bool(std::istream&, _self&)>, _self&> str)
{
    getline(std::cin, str);
}

//]

//[basic
//` (For the source of the examples in this section see
//` [@boost:/libs/type_erasure/example/basic.cpp basic.cpp])
//` [basic1]
//` [basic2]
//` [basic3]
//]
