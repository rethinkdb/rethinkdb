// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: overload.cpp 82858 2013-02-13 18:08:57Z steven_watanabe $

#include <boost/type_erasure/concept_interface.hpp>
#include <boost/type_erasure/param.hpp>
#include <boost/type_erasure/derived.hpp>
#include <boost/type_erasure/is_placeholder.hpp>
#include <boost/utility/enable_if.hpp>

namespace mpl = boost::mpl;
using namespace boost::type_erasure;

//[overload1
/*`
    __concept_interface allows us to inject arbitrary declarations
    into an __any.  This is very flexible, but there are some pitfalls
    to watch out for.  Sometimes we want to use the same concept several
    times with different parameters.  Specializing __concept_interface
    in a way that handles overloads correctly is a bit tricky.
    Given a concept foo, we'd like the following to work:

    ``
        any<
            mpl::vector<
                foo<_self, int>,
                foo<_self, double>,
                copy_constructible<>
            >
        > x = ...;
        x.foo(1);   // calls foo(int)
        x.foo(1.0); // calls foo(double)
    ``

    Because __concept_interface creates a linear
    inheritance chain, without some extra work,
    one overload of foo will hide the other.

    Here are the techniques that I found work reliably.
    
    For member functions I couldn't find a way to
    avoid using two specializations.
*/

template<class T, class U>
struct foo
{
    static void apply(T& t, const U& u) { t.foo(u); }
};

namespace boost {
namespace type_erasure {

template<class T, class U, class Base, class Enable>
struct concept_interface< ::foo<T, U>, Base, T, Enable> : Base
{
    typedef void _fun_defined;
    void foo(typename as_param<Base, const U&>::type arg)
    {
        call(::foo<T, U>(), *this, arg);
    }
};

template<class T, class U, class Base>
struct concept_interface< ::foo<T, U>, Base, T, typename Base::_fun_defined> : Base
{
    using Base::foo;
    void foo(typename as_param<Base, const U&>::type arg)
    {
        call(::foo<T, U>(), *this, arg);
    }
};

}
}

/*`
    This uses SFINAE to detect whether a using declaration is
    needed.  Note that the fourth argument of __concept_interface
    is a dummy parameter which is always void and is
    intended to be used for SFINAE.
    Another solution to the problem that I've used
    in the past is to inject a dummy declaration of `fun`
    and always put in a using declaration.  This is an
    inferior solution for several reasons.  It requires an
    extra interface to add the dummy overload.  It also
    means that `fun` is always overloaded, even if the
    user only asked for one overload.  This makes it
    harder to take the address of fun.

    Note that while using SFINAE requires some code
    to be duplicated, the amount of code that has to
    be duplicated is relatively small, since the implementation
    of __concept_interface is usually a one liner.  It's
    a bit annoying, but I believe it's an acceptable cost
    in lieu of a better solution.
*/

//]
//[overload2
/*`
    For free functions you can use inline friends.
*/

template<class T, class U>
struct bar_concept
{
    static void apply(T& t, const U& u) { bar(t, u); }
};

namespace boost {
namespace type_erasure {

template<class T, class U, class Base>
struct concept_interface< ::bar_concept<T, U>, Base, T> : Base
{
    friend void bar(typename derived<Base>::type& t, typename as_param<Base, const U&>::type u)
    {
        call(::bar_concept<T, U>(), t, u);
    }
};

template<class T, class U, class Base>
struct concept_interface< ::bar_concept<T, U>, Base, U, typename boost::disable_if<is_placeholder<T> >::type> : Base
{
    using Base::bar;
    friend void bar(T& t, const typename derived<Base>::type& u)
    {
        call(::bar_concept<T, U>(), t, u);
    }
};

}
}

/*`
    Basically we have to specialize __concept_interface once for
    each argument to make sure that an overload is injected into
    the first argument that's a placeholder.
    As you might have noticed, the argument types are a bit tricky.
    In the first specialization, the first argument uses __derived
    instead of __as_param. The reason for this is that if we used
    __as_param, then we could end up violating the one definition
    rule by defining the same function twice.  Similarly, we use
    SFINAE in the second specialization to make sure that bar is
    only defined once when both arguments are placeholders.  It's
    possible to merge the two specializations with a bit of metaprogramming,
    but unless you have a lot of arguments, it's probably not
    worth while.
*/

//]

//[overload
//` (For the source of the examples in this section see
//` [@boost:/libs/type_erasure/example/overload.cpp overload.cpp])
//` [overload1]
//` [overload2]
//]
