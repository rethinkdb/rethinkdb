//-----------------------------------------------------------------------------
// boost-libs variant/test/variant_reference_test.cpp source file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2003
// Eric Friedman, Itay Maman
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "boost/variant.hpp"
#include "boost/test/minimal.hpp"

#include "boost/mpl/bool.hpp"
#include "boost/type_traits/add_reference.hpp"
#include "boost/type_traits/is_pointer.hpp"

/////
// support types and functions

struct base_t { };
struct derived_t : base_t { };

template <typename Base, typename Derived>
bool check_base_derived(Base* b, Derived* d, long)
{
    return b == d;
}

template <typename Base, typename Derived>
bool check_base_derived(Base& b, Derived& d, int)
{
    return &b == &d;
}

template <typename T>
    typename boost::add_reference<T>::type
wknd_get(boost::variant<T&>& var, long)
{
    return boost::get<T>(var);
}

template <typename T>
    typename boost::add_reference<T>::type
wknd_get(boost::variant<T>& var, int)
{
    return boost::get<T>(var);
}

/////
// test functions

template <typename T>
void test_reference_content(T& t, const T& value1, const T& value2)
{
    BOOST_CHECK( !(value1 == value2) );

    /////

    boost::variant< T& > var(t);
    BOOST_CHECK(( boost::get<T>(&var) == &t ));

    t = value1;
    BOOST_CHECK(( boost::get<T>(var) == value1 ));

    /////

    boost::variant< T > var2(var);
    BOOST_CHECK(( boost::get<T>(var2) == value1 ));

    t = value2;
    BOOST_CHECK(( boost::get<T>(var2) == value1 ));
}

template <typename Base, typename Derived>
void base_derived_test(Derived d)
{
    typedef typename boost::is_pointer<Base>::type is_ptr;

    Base b(d);
    BOOST_CHECK((check_base_derived(
          b
        , d
        , 1L
        )));

    boost::variant<Base> base_var(d);
    BOOST_CHECK((check_base_derived(
          wknd_get(base_var, 1L)
        , d
        , 1L
        )));

    boost::variant<Derived> derived_var(d);
    boost::variant<Base> base_from_derived_var(derived_var);
    BOOST_CHECK((check_base_derived(
          wknd_get(base_from_derived_var, 1L)
        , wknd_get(derived_var, 1L)
        , 1L
        )));
}

int test_main(int , char* [])
{
    int i = 0;
    test_reference_content(i, 1, 2);

    /////

    derived_t d;
    base_derived_test< int&,int >(i);
    base_derived_test< base_t*,derived_t* >(&d);
    base_derived_test< base_t&,derived_t& >(d);

    return boost::exit_success;
}
