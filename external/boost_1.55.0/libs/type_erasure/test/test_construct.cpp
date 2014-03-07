// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_construct.cpp 83777 2013-04-06 14:49:03Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/tuple.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/binding_of.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/tuple/tuple.hpp>
#include <vector>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::type_erasure;

template<class T = _self>
struct common : ::boost::mpl::vector<
    copy_constructible<T>,
    typeid_<T>
> {};

BOOST_AUTO_TEST_CASE(test_implicit) {
    any<common<> > x = 1;
    BOOST_CHECK_EQUAL(any_cast<int>(x), 1);
}

void func() {}

BOOST_AUTO_TEST_CASE(test_decay) {
    char array[] = "Hello World!";
    const char carray[] = "Hello World!";

    any<common<> > x1(array);
    any<common<> > y1(func);
    any<common<> > z1(carray);
    BOOST_CHECK_EQUAL(any_cast<char *>(x1), &array[0]);
    BOOST_CHECK(any_cast<void(*)()>(y1) == &func);
    BOOST_CHECK_EQUAL(any_cast<const char *>(z1), &carray[0]);

    any<common<> > x2(array, make_binding<boost::mpl::map<boost::mpl::pair<_self, char *> > >());
    any<common<> > y2(func, make_binding<boost::mpl::map<boost::mpl::pair<_self, void(*)()> > >());
    any<common<> > z2(carray, make_binding<boost::mpl::map<boost::mpl::pair<_self, const char *> > >());
    BOOST_CHECK_EQUAL(any_cast<char *>(x2), &array[0]);
    BOOST_CHECK(any_cast<void(*)()>(y2) == &func);
    BOOST_CHECK_EQUAL(any_cast<const char *>(z2), &carray[0]);

    static_binding<boost::mpl::map<boost::mpl::pair<_self, char *> > > bx3;
    static_binding<boost::mpl::map<boost::mpl::pair<_self, void (*)()> > > by3;
    static_binding<boost::mpl::map<boost::mpl::pair<_self, const char *> > > bz3;
    any<common<> > x3(array, bx3);
    any<common<> > y3(func, by3);
    any<common<> > z3(carray, bz3);
    BOOST_CHECK_EQUAL(any_cast<char *>(x3), &array[0]);
    BOOST_CHECK(any_cast<void(*)()>(y3) == &func);
    BOOST_CHECK_EQUAL(any_cast<const char *>(z3), &carray[0]);
}

enum {
    lvalue,
    const_lvalue,
    rvalue
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
        = lvalue
#endif
};

template<class T>
int make_arg_type();

template<>
int make_arg_type<int>() { return rvalue; }
template<>
int make_arg_type<int&>() { return lvalue; }
template<>
int make_arg_type<const int&>() { return const_lvalue; }

enum { id_int = 4, id_copy = 8 };

std::vector<int> make_vector() { return std::vector<int>(); }

template<class T>
std::vector<T> make_vector(T t0) {
    std::vector<T> result;
    result.push_back(t0);
    return result;
}
template<class T>
std::vector<T> make_vector(T t0, T t1) {
    std::vector<T> result;
    result.push_back(t0);
    result.push_back(t1);
    return result;
}

struct test_class
{

    test_class() {}

    test_class(const test_class &)
      : args(make_vector(const_lvalue | id_copy))
    {}

    test_class(test_class &)
      : args(make_vector(lvalue | id_copy))
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

    test_class(test_class &&)
      : args(make_vector(rvalue | id_copy))
    {}

    template<class T0>
    test_class(T0&& t0)
      : args(make_vector(t0 | make_arg_type<T0>()))
    {}

    template<class T0, class T1>
    test_class(T0&& t0, T1&& t1)
      : args(make_vector(t0 | make_arg_type<T0>(), t1 | make_arg_type<T1>()))
    {}

#else

    test_class(int& i0)
      : args(make_vector(i0 | lvalue))
    {}
    test_class(const int& i0)
      : args(make_vector(i0 | const_lvalue))
    {}
    test_class(int& i0, int& i1)
      : args(make_vector(i0 | lvalue, i1 | lvalue))
    {}
    test_class(int& i0, const int& i1)
      : args(make_vector(i0 | lvalue, i1 | const_lvalue))
    {}
    test_class(const int& i0, int& i1)
      : args(make_vector(i0 | const_lvalue, i1 | lvalue))
    {}
    test_class(const int& i0, const int& i1)
      : args(make_vector(i0 | const_lvalue, i1 | const_lvalue))
    {}

#endif
    std::vector<int> args;
};

template<class T>
struct make_arg_impl;

template<>
struct make_arg_impl<int>
{
    static int apply()
    {
        return id_int;
    }
};

template<class Concept>
struct make_arg_impl<binding<Concept> >
{
    static binding<Concept> apply()
    {
        return make_binding< ::boost::mpl::map<
            ::boost::mpl::pair<_a, test_class>,
            ::boost::mpl::pair<_b, int>
        > >();
    }
};

template<class Concept>
struct make_arg_impl<any<Concept, _a> >
{
    static any<Concept, _a> apply()
    {
        return any<Concept, _a>(
            test_class(),
            make_binding< ::boost::mpl::map<
                ::boost::mpl::pair<_a, test_class>,
                ::boost::mpl::pair<_b, int>
        > >());
    }
};

template<class Concept>
struct make_arg_impl<any<Concept, _b> >
{
    static any<Concept, _b> apply()
    {
        return any<Concept, _b>(
            (int)id_int,
            make_binding< ::boost::mpl::map<
                ::boost::mpl::pair<_a, test_class>,
                ::boost::mpl::pair<_b, int>
        > >());
    }
};

template<class Concept, class T>
struct make_arg_impl<any<Concept, T&> >
{
    static any<Concept, T&> apply()
    {
        return make_arg_impl<any<Concept, T>&>::apply();
    }
};

template<class Concept, class T>
struct make_arg_impl<any<Concept, const T&> >
{
    static any<Concept, const T&> apply()
    {
        return make_arg_impl<any<Concept, T>&>::apply();
    }
};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<class Concept, class T>
struct make_arg_impl<any<Concept, T&&> >
{
    static any<Concept, T&&> apply()
    {
        return std::move(make_arg_impl<any<Concept, T>&>::apply());
    }
};

#endif

template<class T>
struct make_arg_impl<const T> : make_arg_impl<T> {};

template<class T>
struct make_arg_impl<T&>
{
    static T& apply()
    {
        static T result = make_arg_impl<T>::apply();
        return result;
    }
};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<class T>
struct make_arg_impl<T&&>
{
    static T&& apply()
    {
        static T result = make_arg_impl<T>::apply();
        return std::move(result);
    }
};

#endif

#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES

template<class T>
T make_arg()
{
    return make_arg_impl<T>::apply();
}

#else

template<class T>
T&& make_arg()
{
    return make_arg_impl<T&&>::apply();
}

#endif

int get_value(int i) { return i; }
template<class T>
int get_value(const T& t) { return any_cast<int>(t); }

template<class Sig, class Args>
struct tester;

template<class Concept, class T>
struct tester<Concept, void(T)>
{
    static std::vector<int> apply()
    {
        any<Concept, _a> x(make_arg<T>());
        const test_class& result = any_cast<const test_class&>(x);
        return result.args;
    }
};

template<class Concept, class T0, class T1>
struct tester<Concept, void(T0, T1)>
{
    static std::vector<int> apply()
    {
        any<Concept, _a> x(make_arg<T0>(), make_arg<T1>());
        const test_class& result = any_cast<const test_class&>(x);
        return result.args;
    }
};

template<class Concept, class T0, class T1, class T2>
struct tester<Concept, void(T0, T1, T2)>
{
    static std::vector<int> apply()
    {
        any<Concept, _a> x(make_arg<T0>(), make_arg<T1>(), make_arg<T2>());
        const test_class& result = any_cast<const test_class&>(x);
        return result.args;
    }
};

#define TEST_CONSTRUCT(sig, args, expected_) \
{\
    typedef ::boost::mpl::vector<\
        common<_a>, \
        common<_b>,\
        constructible<sig>\
    > C;\
    std::vector<int> result = tester<C, void args>::apply();\
    std::vector<int> expected = make_vector expected_;\
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), \
        expected.begin(), expected.end());\
}

BOOST_AUTO_TEST_CASE(test_default)
{
    TEST_CONSTRUCT(_a(), (binding<C>), ());
    TEST_CONSTRUCT(_a(), (binding<C>&), ());
    TEST_CONSTRUCT(_a(), (const binding<C>&), ());
}

// test all forms of direct construction that take 1 argument
BOOST_AUTO_TEST_CASE(test_construct1)
{
    // construction from int
    TEST_CONSTRUCT(_a(int&), (binding<C>, int&), (lvalue | id_int));
    TEST_CONSTRUCT(_a(int&), (binding<C>&, int&), (lvalue | id_int));
    TEST_CONSTRUCT(_a(int&), (const binding<C>&, int&), (lvalue | id_int));

    TEST_CONSTRUCT(_a(const int&), (binding<C>, int), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const int&), (binding<C>, int&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const int&), (binding<C>, const int&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const int&), (binding<C>&, int), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const int&), (binding<C>&, int&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const int&), (binding<C>&, const int&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const int&), (const binding<C>&, int), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const int&), (const binding<C>&, int&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const int&), (const binding<C>&, const int&), (const_lvalue | id_int));

    TEST_CONSTRUCT(_a(int), (binding<C>, int), (rvalue | id_int));
    TEST_CONSTRUCT(_a(int), (binding<C>, int&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(int), (binding<C>, const int&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(int), (binding<C>&, int), (rvalue | id_int));
    TEST_CONSTRUCT(_a(int), (binding<C>&, int&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(int), (binding<C>&, const int&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(int), (const binding<C>&, int), (rvalue | id_int));
    TEST_CONSTRUCT(_a(int), (const binding<C>&, int&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(int), (const binding<C>&, const int&), (rvalue | id_int));

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    TEST_CONSTRUCT(_a(int&&), (binding<C>, int), (rvalue | id_int));
    TEST_CONSTRUCT(_a(int&&), (binding<C>&, int), (rvalue | id_int));
    TEST_CONSTRUCT(_a(int&&), (const binding<C>&, int), (rvalue | id_int));
#endif

    // Test same any type

#ifndef BOOST_NO_FUNCTION_REFERENCE_QUALIFIERS
    // ambiguous with the copy constructor in C++03
    TEST_CONSTRUCT(_a(_a&), (any<C, _a>&), (lvalue | id_copy));
    TEST_CONSTRUCT(_a(_a&), (binding<C>, any<C, _a>&), (lvalue | id_copy));
    TEST_CONSTRUCT(_a(_a&), (binding<C>&, any<C, _a>&), (lvalue | id_copy));
    TEST_CONSTRUCT(_a(_a&), (const binding<C>&, any<C, _a>&), (lvalue | id_copy));
#endif

    TEST_CONSTRUCT(_a(const _a&), (any<C, _a>), (const_lvalue | id_copy));
    TEST_CONSTRUCT(_a(const _a&), (any<C, _a>&), (const_lvalue | id_copy));
    TEST_CONSTRUCT(_a(const _a&), (const any<C, _a>&), (const_lvalue | id_copy));
    TEST_CONSTRUCT(_a(const _a&), (binding<C>, any<C, _a>), (const_lvalue | id_copy));
    TEST_CONSTRUCT(_a(const _a&), (binding<C>, any<C, _a>&), (const_lvalue | id_copy));
    TEST_CONSTRUCT(_a(const _a&), (binding<C>, const any<C, _a>&), (const_lvalue | id_copy));
    TEST_CONSTRUCT(_a(const _a&), (binding<C>&, any<C, _a>), (const_lvalue | id_copy));
    TEST_CONSTRUCT(_a(const _a&), (binding<C>&, any<C, _a>&), (const_lvalue | id_copy));
    TEST_CONSTRUCT(_a(const _a&), (binding<C>&, const any<C, _a>&), (const_lvalue | id_copy));
    TEST_CONSTRUCT(_a(const _a&), (const binding<C>&, any<C, _a>), (const_lvalue | id_copy));
    TEST_CONSTRUCT(_a(const _a&), (const binding<C>&, any<C, _a>&), (const_lvalue | id_copy));
    TEST_CONSTRUCT(_a(const _a&), (const binding<C>&, const any<C, _a>&), (const_lvalue | id_copy));
    
#ifndef BOOST_NO_FUNCTION_REFERENCE_QUALIFIERS

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    TEST_CONSTRUCT(_a(_a&&), (any<C, _a>), (rvalue | id_copy));
    TEST_CONSTRUCT(_a(_a&&), (binding<C>, any<C, _a>), (rvalue | id_copy));
    TEST_CONSTRUCT(_a(_a&&), (binding<C>&, any<C, _a>), (rvalue | id_copy));
    TEST_CONSTRUCT(_a(_a&&), (const binding<C>&, any<C, _a>), (rvalue | id_copy));
#endif

#endif

    // test other any type
    TEST_CONSTRUCT(_a(_b&), (any<C, _b>&), (lvalue | id_int));
    TEST_CONSTRUCT(_a(_b&), (binding<C>, any<C, _b>&), (lvalue | id_int));
    TEST_CONSTRUCT(_a(_b&), (binding<C>&, any<C, _b>&), (lvalue | id_int));
    TEST_CONSTRUCT(_a(_b&), (const binding<C>&, any<C, _b>&), (lvalue | id_int));

    TEST_CONSTRUCT(_a(const _b&), (any<C, _b>), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (any<C, _b>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (const any<C, _b>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>, any<C, _b>), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>, any<C, _b>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>, const any<C, _b>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>&, any<C, _b>), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>&, any<C, _b>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>&, const any<C, _b>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (const binding<C>&, any<C, _b>), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (const binding<C>&, any<C, _b>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (const binding<C>&, const any<C, _b>&), (const_lvalue | id_int));

    TEST_CONSTRUCT(_a(_b), (any<C, _b>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (any<C, _b>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (const any<C, _b>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>, any<C, _b>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>, any<C, _b>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>, const any<C, _b>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>&, any<C, _b>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>&, any<C, _b>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>&, const any<C, _b>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (const binding<C>&, any<C, _b>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (const binding<C>&, any<C, _b>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (const binding<C>&, const any<C, _b>&), (rvalue | id_int));
    
#ifndef BOOST_NO_FUNCTION_REFERENCE_QUALIFIERS

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    TEST_CONSTRUCT(_a(_b&&), (any<C, _b>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b&&), (binding<C>, any<C, _b>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b&&), (binding<C>&, any<C, _b>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b&&), (const binding<C>&, any<C, _b>), (rvalue | id_int));
#endif

#endif

    // test any reference type
    TEST_CONSTRUCT(_a(_b&), (any<C, _b&>), (lvalue | id_int));
    TEST_CONSTRUCT(_a(_b&), (any<C, _b&>&), (lvalue | id_int));
    TEST_CONSTRUCT(_a(_b&), (const any<C, _b&>&), (lvalue | id_int));
    TEST_CONSTRUCT(_a(_b&), (binding<C>, any<C, _b&>), (lvalue | id_int));
    TEST_CONSTRUCT(_a(_b&), (binding<C>, any<C, _b&>&), (lvalue | id_int));
    TEST_CONSTRUCT(_a(_b&), (binding<C>, const any<C, _b&>&), (lvalue | id_int));
    TEST_CONSTRUCT(_a(_b&), (binding<C>&, any<C, _b&>), (lvalue | id_int));
    TEST_CONSTRUCT(_a(_b&), (binding<C>&, any<C, _b&>&), (lvalue | id_int));
    TEST_CONSTRUCT(_a(_b&), (binding<C>&, const any<C, _b&>&), (lvalue | id_int));
    TEST_CONSTRUCT(_a(_b&), (const binding<C>&, any<C, _b&>), (lvalue | id_int));
    TEST_CONSTRUCT(_a(_b&), (const binding<C>&, any<C, _b&>&), (lvalue | id_int));
    TEST_CONSTRUCT(_a(_b&), (const binding<C>&, const any<C, _b&>&), (lvalue | id_int));

    TEST_CONSTRUCT(_a(const _b&), (any<C, _b&>), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (any<C, _b&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (const any<C, _b&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>, any<C, _b&>), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>, any<C, _b&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>, const any<C, _b&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>&, any<C, _b&>), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>&, any<C, _b&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>&, const any<C, _b&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (const binding<C>&, any<C, _b&>), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (const binding<C>&, any<C, _b&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (const binding<C>&, const any<C, _b&>&), (const_lvalue | id_int));

    TEST_CONSTRUCT(_a(_b), (any<C, _b&>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (any<C, _b&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (const any<C, _b&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>, any<C, _b&>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>, any<C, _b&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>, const any<C, _b&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>&, any<C, _b&>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>&, any<C, _b&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>&, const any<C, _b&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (const binding<C>&, any<C, _b&>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (const binding<C>&, any<C, _b&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (const binding<C>&, const any<C, _b&>&), (rvalue | id_int));

    // test const any reference type
    TEST_CONSTRUCT(_a(const _b&), (any<C, const _b&>), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (any<C, const _b&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (const any<C, const _b&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>, any<C, const _b&>), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>, any<C, const _b&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>, const any<C, const _b&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>&, any<C, const _b&>), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>&, any<C, const _b&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>&, const any<C, const _b&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (const binding<C>&, any<C, const _b&>), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (const binding<C>&, any<C, const _b&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (const binding<C>&, const any<C, const _b&>&), (const_lvalue | id_int));

    TEST_CONSTRUCT(_a(_b), (any<C, const _b&>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (any<C, const _b&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (const any<C, const _b&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>, any<C, const _b&>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>, any<C, const _b&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>, const any<C, const _b&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>&, any<C, const _b&>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>&, any<C, const _b&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>&, const any<C, const _b&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (const binding<C>&, any<C, const _b&>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (const binding<C>&, any<C, const _b&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (const binding<C>&, const any<C, const _b&>&), (rvalue | id_int));
    
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

    // test any rvalue reference type
    TEST_CONSTRUCT(_a(const _b&), (any<C, _b&&>), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (any<C, _b&&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (const any<C, _b&&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>, any<C, _b&&>), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>, any<C, _b&&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>, const any<C, _b&&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>&, any<C, _b&&>), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>&, any<C, _b&&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (binding<C>&, const any<C, _b&&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (const binding<C>&, any<C, _b&&>), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (const binding<C>&, any<C, _b&&>&), (const_lvalue | id_int));
    TEST_CONSTRUCT(_a(const _b&), (const binding<C>&, const any<C, _b&&>&), (const_lvalue | id_int));

    TEST_CONSTRUCT(_a(_b), (any<C, _b&&>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (any<C, _b&&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (const any<C, _b&&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>, any<C, _b&&>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>, any<C, _b&&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>, const any<C, _b&&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>&, any<C, _b&&>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>&, any<C, _b&&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (binding<C>&, const any<C, _b&&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (const binding<C>&, any<C, _b&&>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (const binding<C>&, any<C, _b&&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b), (const binding<C>&, const any<C, _b&&>&), (rvalue | id_int));

    TEST_CONSTRUCT(_a(_b&&), (any<C, _b&&>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b&&), (any<C, _b&&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b&&), (const any<C, _b&&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b&&), (binding<C>, any<C, _b&&>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b&&), (binding<C>, any<C, _b&&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b&&), (binding<C>, const any<C, _b&&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b&&), (binding<C>&, any<C, _b&&>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b&&), (binding<C>&, any<C, _b&&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b&&), (binding<C>&, const any<C, _b&&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b&&), (const binding<C>&, any<C, _b&&>), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b&&), (const binding<C>&, any<C, _b&&>&), (rvalue | id_int));
    TEST_CONSTRUCT(_a(_b&&), (const binding<C>&, const any<C, _b&&>&), (rvalue | id_int));

#endif

}

// test constructors with 2 parameters
BOOST_AUTO_TEST_CASE(test_construct2)
{
    TEST_CONSTRUCT(_a(int, int), (binding<C>, int, int), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>, int, int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>, int, const int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>, int&, int), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>, int&, int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>, int&, const int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>, const int&, int), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>, const int&, int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>, const int&, const int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>&, int, int), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>&, int, int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>&, int, const int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>&, int&, int), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>&, int&, int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>&, int&, const int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>&, const int&, int), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>&, const int&, int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (binding<C>&, const int&, const int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (const binding<C>&, int, int), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (const binding<C>&, int, int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (const binding<C>&, int, const int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (const binding<C>&, int&, int), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (const binding<C>&, int&, int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (const binding<C>&, int&, const int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (const binding<C>&, const int&, int), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (const binding<C>&, const int&, int&), (rvalue | id_int, rvalue | id_int));
    TEST_CONSTRUCT(_a(int, int), (const binding<C>&, const int&, const int&), (rvalue | id_int, rvalue | id_int));
}

BOOST_AUTO_TEST_CASE(test_overload)
{
    typedef ::boost::mpl::vector<
        common<_a>,
        common<_b>,
        constructible<_a(_b)>,
        constructible<_a(std::size_t)>
    > test_concept;
    typedef ::boost::mpl::map<
        ::boost::mpl::pair<_a, std::vector<int> >,
        ::boost::mpl::pair<_b, std::size_t>
    > types;
    binding<test_concept> table = make_binding<types>();
    any<test_concept, _b> x(static_cast<std::size_t>(10), make_binding<types>());
    any<test_concept, _a> y(x);
    any<test_concept, _a> z(table, 17);
    std::vector<int> vec1(any_cast<std::vector<int> >(y));
    BOOST_CHECK_EQUAL(vec1.size(), 10u);
    std::vector<int> vec2(any_cast<std::vector<int> >(z));
    BOOST_CHECK_EQUAL(vec2.size(), 17u);
}

template<class T>
T as_rvalue(const T& arg) { return arg; }
template<class T>
const T& as_const(const T& arg) { return arg; }

BOOST_AUTO_TEST_CASE(test_from_int_with_binding)
{
    typedef ::boost::mpl::vector<common<> > test_concept;
    static_binding<boost::mpl::map<boost::mpl::pair<_self, int> > > binding =
        make_binding<boost::mpl::map<boost::mpl::pair<_self, int> > >();
    int value = 4;

    any<test_concept> x1(value, binding);
    BOOST_CHECK_EQUAL(any_cast<int>(x1), 4);
    any<test_concept> x2(value, as_rvalue(binding));
    BOOST_CHECK_EQUAL(any_cast<int>(x2), 4);
    any<test_concept> x3(value, as_const(binding));
    BOOST_CHECK_EQUAL(any_cast<int>(x3), 4);
    
    any<test_concept> y1(as_rvalue(value), binding);
    BOOST_CHECK_EQUAL(any_cast<int>(y1), 4);
    any<test_concept> y2(as_rvalue(value), as_rvalue(binding));
    BOOST_CHECK_EQUAL(any_cast<int>(y2), 4);
    any<test_concept> y3(as_rvalue(value), as_const(binding));
    BOOST_CHECK_EQUAL(any_cast<int>(y3), 4);
    
    any<test_concept> z1(as_const(value), binding);
    BOOST_CHECK_EQUAL(any_cast<int>(z1), 4);
    any<test_concept> z2(as_const(value), as_rvalue(binding));
    BOOST_CHECK_EQUAL(any_cast<int>(z2), 4);
    any<test_concept> z3(as_const(value), as_const(binding));
    BOOST_CHECK_EQUAL(any_cast<int>(z3), 4);
}

BOOST_AUTO_TEST_CASE(test_copy)
{
    typedef ::boost::mpl::vector<common<> > test_concept;
    any<test_concept> x(4);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
    any<test_concept> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<test_concept> z(as_rvalue(x));
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<test_concept> w(as_const(x));
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_copy_implicit)
{
    typedef ::boost::mpl::vector<common<>, common<_a> > test_concept;
    any<test_concept> x(4, make_binding< ::boost::mpl::map< ::boost::mpl::pair<_self, int>, ::boost::mpl::pair<_a, int> > >());
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);

    any<test_concept> y = x;
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<test_concept> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<test_concept> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_convert)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > src_concept;
    typedef ::boost::mpl::vector<common<> > dst_concept;
    any<src_concept> x(4);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
    any<dst_concept> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<dst_concept> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<dst_concept> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_rebind)
{
    typedef ::boost::mpl::vector<common<> > src_concept;
    typedef ::boost::mpl::vector<common<_a> > dst_concept;
    any<src_concept> x(4);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
    any<dst_concept, _a> y = x;
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<dst_concept, _a> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<dst_concept, _a> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_rebind_and_convert)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > src_concept;
    typedef ::boost::mpl::vector<common<_a> > dst_concept;
    any<src_concept> x(4);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
    any<dst_concept, _a> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<dst_concept, _a> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<dst_concept, _a> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_rebind_and_convert_with_binding)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > src_concept;
    typedef ::boost::mpl::vector<common<_a> > dst_concept;
    typedef ::boost::mpl::map<boost::mpl::pair<_a, _self> > map;
    typedef ::boost::mpl::map<boost::mpl::pair<_a, int> > types;

    static_binding<map> s_table(make_binding<map>());
    binding<dst_concept> table(make_binding<types>());
    
    any<src_concept> x(4);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);

    // lvalues
    any<dst_concept, _a> y1(x, s_table);
    BOOST_CHECK_EQUAL(any_cast<int>(y1), 4);
    any<dst_concept, _a> y2(x, as_rvalue(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(y2), 4);
    any<dst_concept, _a> y3(x, as_const(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(y3), 4);
    any<dst_concept, _a> z1(x, table);
    BOOST_CHECK_EQUAL(any_cast<int>(z1), 4);
    any<dst_concept, _a> z2(x, as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(z2), 4);
    any<dst_concept, _a> z3(x, as_const(table));
    BOOST_CHECK_EQUAL(any_cast<int>(z3), 4);
    
    // rvalues
    any<dst_concept, _a> ry1(as_rvalue(x), s_table);
    BOOST_CHECK_EQUAL(any_cast<int>(ry1), 4);
    any<dst_concept, _a> ry2(as_rvalue(x), as_rvalue(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(ry2), 4);
    any<dst_concept, _a> ry3(as_rvalue(x), as_const(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(ry3), 4);
    any<dst_concept, _a> rz1(as_rvalue(x), table);
    BOOST_CHECK_EQUAL(any_cast<int>(rz1), 4);
    any<dst_concept, _a> rz2(as_rvalue(x), as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(rz2), 4);
    any<dst_concept, _a> rz3(as_rvalue(x), as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(rz3), 4);
    
    // const lvalues
    any<dst_concept, _a> cy1(as_const(x), s_table);
    BOOST_CHECK_EQUAL(any_cast<int>(cy1), 4);
    any<dst_concept, _a> cy2(as_const(x), as_rvalue(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(cy2), 4);
    any<dst_concept, _a> cy3(as_const(x), as_const(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(cy3), 4);
    any<dst_concept, _a> cz1(as_const(x), table);
    BOOST_CHECK_EQUAL(any_cast<int>(cz1), 4);
    any<dst_concept, _a> cz2(as_const(x), as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(cz2), 4);
    any<dst_concept, _a> cz3(as_const(x), as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(cz3), 4);
}

BOOST_AUTO_TEST_CASE(test_copy_from_ref)
{
    typedef ::boost::mpl::vector<common<> > test_concept;
    int i = 4;
    any<test_concept, _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
    any<test_concept> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<test_concept> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<test_concept> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_convert_from_ref)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > src_concept;
    typedef ::boost::mpl::vector<common<> > dst_concept;
    int i = 4;
    any<src_concept, _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
    any<dst_concept> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<dst_concept> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<dst_concept> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_rebind_from_ref)
{
    typedef ::boost::mpl::vector<common<> > src_concept;
    typedef ::boost::mpl::vector<common<_a> > dst_concept;
    int i = 4;
    any<src_concept, _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
    any<dst_concept, _a> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<dst_concept, _a> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<dst_concept, _a> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_rebind_and_convert_from_ref)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > src_concept;
    typedef ::boost::mpl::vector<common<_a> > dst_concept;
    int i = 4;
    any<src_concept, _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
    any<dst_concept, _a> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<dst_concept, _a> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<dst_concept, _a> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_rebind_and_convert_with_binding_from_ref)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > src_concept;
    typedef ::boost::mpl::vector<common<_a> > dst_concept;
    typedef ::boost::mpl::map<boost::mpl::pair<_a, _self> > map;
    typedef ::boost::mpl::map<boost::mpl::pair<_a, int> > types;
    
    static_binding<map> s_table(make_binding<map>());
    binding<dst_concept> table(make_binding<types>());
    
    int i = 4;
    any<src_concept, _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);

    // lvalues
    any<dst_concept, _a> y1(x, s_table);
    BOOST_CHECK_EQUAL(any_cast<int>(y1), 4);
    any<dst_concept, _a> y2(x, as_rvalue(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(y2), 4);
    any<dst_concept, _a> y3(x, as_const(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(y3), 4);
    any<dst_concept, _a> z1(x, table);
    BOOST_CHECK_EQUAL(any_cast<int>(z1), 4);
    any<dst_concept, _a> z2(x, as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(z2), 4);
    any<dst_concept, _a> z3(x, as_const(table));
    BOOST_CHECK_EQUAL(any_cast<int>(z3), 4);
    
    // rvalues
    any<dst_concept, _a> ry1(as_rvalue(x), s_table);
    BOOST_CHECK_EQUAL(any_cast<int>(ry1), 4);
    any<dst_concept, _a> ry2(as_rvalue(x), as_rvalue(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(ry2), 4);
    any<dst_concept, _a> ry3(as_rvalue(x), as_const(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(ry3), 4);
    any<dst_concept, _a> rz1(as_rvalue(x), table);
    BOOST_CHECK_EQUAL(any_cast<int>(rz1), 4);
    any<dst_concept, _a> rz2(as_rvalue(x), as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(rz2), 4);
    any<dst_concept, _a> rz3(as_rvalue(x), as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(rz3), 4);
    
    // const lvalues
    any<dst_concept, _a> cy1(as_const(x), s_table);
    BOOST_CHECK_EQUAL(any_cast<int>(cy1), 4);
    any<dst_concept, _a> cy2(as_const(x), as_rvalue(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(cy2), 4);
    any<dst_concept, _a> cy3(as_const(x), as_const(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(cy3), 4);
    any<dst_concept, _a> cz1(as_const(x), table);
    BOOST_CHECK_EQUAL(any_cast<int>(cz1), 4);
    any<dst_concept, _a> cz2(as_const(x), as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(cz2), 4);
    any<dst_concept, _a> cz3(as_const(x), as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(cz3), 4);
}

BOOST_AUTO_TEST_CASE(test_copy_from_cref)
{
    typedef ::boost::mpl::vector<common<> > test_concept;
    int i = 4;
    any<test_concept, const _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
    any<test_concept> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<test_concept> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<test_concept> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_convert_from_cref)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > src_concept;
    typedef ::boost::mpl::vector<common<> > dst_concept;
    int i = 4;
    any<src_concept, const _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
    any<dst_concept> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<dst_concept> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<dst_concept> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_rebind_from_cref)
{
    typedef ::boost::mpl::vector<common<> > src_concept;
    typedef ::boost::mpl::vector<common<_a> > dst_concept;
    int i = 4;
    any<src_concept, const _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
    any<dst_concept, _a> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<dst_concept, _a> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<dst_concept, _a> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_rebind_and_convert_from_cref)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > src_concept;
    typedef ::boost::mpl::vector<common<_a> > dst_concept;
    int i = 4;
    any<src_concept, const _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
    any<dst_concept, _a> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<dst_concept, _a> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<dst_concept, _a> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_rebind_and_convert_with_binding_from_cref)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > src_concept;
    typedef ::boost::mpl::vector<common<_a> > dst_concept;
    typedef ::boost::mpl::map<boost::mpl::pair<_a, _self> > map;
    typedef ::boost::mpl::map<boost::mpl::pair<_a, int> > types;
    
    static_binding<map> s_table(make_binding<map>());
    binding<dst_concept> table(make_binding<types>());
    
    int i = 4;
    any<src_concept, const _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);

    // lvalues
    any<dst_concept, _a> y1(x, s_table);
    BOOST_CHECK_EQUAL(any_cast<int>(y1), 4);
    any<dst_concept, _a> y2(x, as_rvalue(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(y2), 4);
    any<dst_concept, _a> y3(x, as_const(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(y3), 4);
    any<dst_concept, _a> z1(x, table);
    BOOST_CHECK_EQUAL(any_cast<int>(z1), 4);
    any<dst_concept, _a> z2(x, as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(z2), 4);
    any<dst_concept, _a> z3(x, as_const(table));
    BOOST_CHECK_EQUAL(any_cast<int>(z3), 4);
    
    // rvalues
    any<dst_concept, _a> ry1(as_rvalue(x), s_table);
    BOOST_CHECK_EQUAL(any_cast<int>(ry1), 4);
    any<dst_concept, _a> ry2(as_rvalue(x), as_rvalue(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(ry2), 4);
    any<dst_concept, _a> ry3(as_rvalue(x), as_const(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(ry3), 4);
    any<dst_concept, _a> rz1(as_rvalue(x), table);
    BOOST_CHECK_EQUAL(any_cast<int>(rz1), 4);
    any<dst_concept, _a> rz2(as_rvalue(x), as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(rz2), 4);
    any<dst_concept, _a> rz3(as_rvalue(x), as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(rz3), 4);
    
    // const lvalues
    any<dst_concept, _a> cy1(as_const(x), s_table);
    BOOST_CHECK_EQUAL(any_cast<int>(cy1), 4);
    any<dst_concept, _a> cy2(as_const(x), as_rvalue(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(cy2), 4);
    any<dst_concept, _a> cy3(as_const(x), as_const(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(cy3), 4);
    any<dst_concept, _a> cz1(as_const(x), table);
    BOOST_CHECK_EQUAL(any_cast<int>(cz1), 4);
    any<dst_concept, _a> cz2(as_const(x), as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(cz2), 4);
    any<dst_concept, _a> cz3(as_const(x), as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(cz3), 4);
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

BOOST_AUTO_TEST_CASE(test_copy_from_rref)
{
    typedef ::boost::mpl::vector<common<> > test_concept;
    int i = 4;
    any<test_concept, _self&&> x(std::move(i));
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
    any<test_concept> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<test_concept> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<test_concept> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_convert_from_rref)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > src_concept;
    typedef ::boost::mpl::vector<common<> > dst_concept;
    int i = 4;
    any<src_concept, _self&&> x(std::move(i));
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
    any<dst_concept> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<dst_concept> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<dst_concept> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_rebind_from_rref)
{
    typedef ::boost::mpl::vector<common<> > src_concept;
    typedef ::boost::mpl::vector<common<_a> > dst_concept;
    int i = 4;
    any<src_concept, _self&&> x(std::move(i));
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
    any<dst_concept, _a> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<dst_concept, _a> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<dst_concept, _a> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_rebind_and_convert_from_rref)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > src_concept;
    typedef ::boost::mpl::vector<common<_a> > dst_concept;
    int i = 4;
    any<src_concept, _self&&> x(std::move(i));
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);
    any<dst_concept, _a> y(x);
    BOOST_CHECK_EQUAL(any_cast<int>(y), 4);
    any<dst_concept, _a> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<int>(z), 4);
    any<dst_concept, _a> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<int>(w), 4);
}

BOOST_AUTO_TEST_CASE(test_rebind_and_convert_with_binding_from_rref)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > src_concept;
    typedef ::boost::mpl::vector<common<_a> > dst_concept;
    typedef ::boost::mpl::map<boost::mpl::pair<_a, _self> > map;
    typedef ::boost::mpl::map<boost::mpl::pair<_a, int> > types;
    
    static_binding<map> s_table(make_binding<map>());
    binding<dst_concept> table(make_binding<types>());
    
    int i = 4;
    any<src_concept, _self&&> x(std::move(i));
    BOOST_CHECK_EQUAL(any_cast<int>(x), 4);

    // lvalues
    any<dst_concept, _a> y1(x, s_table);
    BOOST_CHECK_EQUAL(any_cast<int>(y1), 4);
    any<dst_concept, _a> y2(x, as_rvalue(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(y2), 4);
    any<dst_concept, _a> y3(x, as_const(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(y3), 4);
    any<dst_concept, _a> z1(x, table);
    BOOST_CHECK_EQUAL(any_cast<int>(z1), 4);
    any<dst_concept, _a> z2(x, as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(z2), 4);
    any<dst_concept, _a> z3(x, as_const(table));
    BOOST_CHECK_EQUAL(any_cast<int>(z3), 4);
    
    // rvalues
    any<dst_concept, _a> ry1(as_rvalue(x), s_table);
    BOOST_CHECK_EQUAL(any_cast<int>(ry1), 4);
    any<dst_concept, _a> ry2(as_rvalue(x), as_rvalue(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(ry2), 4);
    any<dst_concept, _a> ry3(as_rvalue(x), as_const(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(ry3), 4);
    any<dst_concept, _a> rz1(as_rvalue(x), table);
    BOOST_CHECK_EQUAL(any_cast<int>(rz1), 4);
    any<dst_concept, _a> rz2(as_rvalue(x), as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(rz2), 4);
    any<dst_concept, _a> rz3(as_rvalue(x), as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(rz3), 4);
    
    // const lvalues
    any<dst_concept, _a> cy1(as_const(x), s_table);
    BOOST_CHECK_EQUAL(any_cast<int>(cy1), 4);
    any<dst_concept, _a> cy2(as_const(x), as_rvalue(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(cy2), 4);
    any<dst_concept, _a> cy3(as_const(x), as_const(s_table));
    BOOST_CHECK_EQUAL(any_cast<int>(cy3), 4);
    any<dst_concept, _a> cz1(as_const(x), table);
    BOOST_CHECK_EQUAL(any_cast<int>(cz1), 4);
    any<dst_concept, _a> cz2(as_const(x), as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(cz2), 4);
    any<dst_concept, _a> cz3(as_const(x), as_rvalue(table));
    BOOST_CHECK_EQUAL(any_cast<int>(cz3), 4);
}

#endif
