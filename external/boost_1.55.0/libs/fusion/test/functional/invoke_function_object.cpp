/*=============================================================================
    Copyright (c) 2005-2006 Joao Abecasis
    Copyright (c) 2006-2007 Tobias Schwinger

    Use modification and distribution are subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#include <boost/fusion/functional/invocation/invoke_function_object.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/type_traits/is_same.hpp>

#include <memory>
#include <boost/noncopyable.hpp>

#include <boost/mpl/int.hpp>

#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/container/list.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/view/single_view.hpp>
#include <boost/fusion/view/iterator_range.hpp>
#include <boost/fusion/iterator/advance.hpp>
#include <boost/fusion/algorithm/transformation/join.hpp>

namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

template <typename T>
inline T const & const_(T const & t)
{
    return t;
}

struct object {};
struct object_nc : boost::noncopyable {};

struct fobj
{
    // Handle nullary separately to exercise result_of support
    template <typename Sig>
    struct result;

    template <class Self, typename T0>
    struct result< Self(T0) >
    {
        typedef int type;
    };

    template <class Self, typename T0, typename T1>
    struct result< Self(T0, T1) >
    {
        typedef int type;
    };

    template <class Self, typename T0, typename T1, typename T2>
    struct result< Self(T0, T1, T2) >
    {
        typedef int type;
    };

    int operator()()       { return 0; }
    int operator()() const { return 1; }

    int operator()(int i)       { return 2 + i; }
    int operator()(int i) const { return 3 + i; }

    int operator()(int i, object &)             { return 4 + i; }
    int operator()(int i, object &) const       { return 5 + i; }
    int operator()(int i, object const &)       { return 6 + i; }
    int operator()(int i, object const &) const { return 7 + i; }

    int operator()(int i, object &, object_nc &)       { return 10 + i; }
    int operator()(int i, object &, object_nc &) const { return 11 + i; }
};

struct nullary_fobj
{
    typedef int result_type;

    int operator()()       { return 0; }
    int operator()() const { return 1; }
};

struct fobj_nc
      : boost::noncopyable
{
    // Handle nullary separately to exercise result_of support
    template <typename T>
    struct result;

    template <class Self, typename T0>
    struct result< Self(T0) >
    {
        typedef int type;
    };

    int operator()(int i)       { return 14 + i; }
    int operator()(int i) const { return 15 + i; }
};

struct nullary_fobj_nc
      : boost::noncopyable
{
    typedef int result_type;

    int operator()()       { return 12; }
    int operator()() const { return 13; }
};


typedef int         element1_type;
typedef object      element2_type;
typedef object_nc & element3_type;

int         element1 = 100;
object      element2 = object();
object_nc   element3;

template <class Sequence>
void test_sequence_n(Sequence & seq, mpl::int_<0>)
{
    // Function Objects

    nullary_fobj f;
    BOOST_TEST(f () == fusion::invoke_function_object(f ,        seq ));
    BOOST_TEST(f () == fusion::invoke_function_object(f , const_(seq)));

    // Note: The function object is taken by value, so we request the copy
    // to be const with an explicit template argument. We can also request
    // the function object to be pased by reference...
    BOOST_TEST(const_(f)() == fusion::invoke_function_object<nullary_fobj const  >(const_(f),        seq ));
    BOOST_TEST(const_(f)() == fusion::invoke_function_object<nullary_fobj const &>(const_(f), const_(seq)));

    nullary_fobj_nc nc_f;
    // ...and we further ensure there is no copying in this case, using a
    // noncopyable function object.
    BOOST_TEST(nc_f () == fusion::invoke_function_object<nullary_fobj_nc &>(nc_f ,        seq ));
    BOOST_TEST(nc_f () == fusion::invoke_function_object<nullary_fobj_nc &>(nc_f , const_(seq)));
    BOOST_TEST(const_(nc_f)() == fusion::invoke_function_object<nullary_fobj_nc const &>(const_(nc_f),        seq ));
    BOOST_TEST(const_(nc_f)() == fusion::invoke_function_object<nullary_fobj_nc const &>(const_(nc_f), const_(seq)));
}

template <class Sequence>
void test_sequence_n(Sequence & seq, mpl::int_<1>)
{
    fobj f;
    BOOST_TEST(f(element1) == fusion::invoke_function_object(f , seq ));
    BOOST_TEST(f(element1) == fusion::invoke_function_object(f , const_(seq)));
    BOOST_TEST(const_(f)(element1) == fusion::invoke_function_object<fobj const  >(const_(f), seq ));
    BOOST_TEST(const_(f)(element1) == fusion::invoke_function_object<fobj const &>(const_(f), const_(seq)));

    fobj_nc nc_f;
    BOOST_TEST(nc_f(element1) == fusion::invoke_function_object<fobj_nc &>(nc_f, seq ));
    BOOST_TEST(nc_f(element1) == fusion::invoke_function_object<fobj_nc &>(nc_f, const_(seq)));
    BOOST_TEST(const_(nc_f)(element1) == fusion::invoke_function_object<fobj_nc const &>(const_(nc_f), seq ));
    BOOST_TEST(const_(nc_f)(element1) == fusion::invoke_function_object<fobj_nc const &>(const_(nc_f), const_(seq)));
}

template <class Sequence>
void test_sequence_n(Sequence & seq, mpl::int_<2>)
{
    fobj f;
    BOOST_TEST(f (element1, element2) == fusion::invoke_function_object(f , seq));
    BOOST_TEST(f (element1, const_(element2)) == fusion::invoke_function_object(f , const_(seq)));
    BOOST_TEST(const_(f)(element1, element2) == fusion::invoke_function_object<fobj const>(const_(f), seq));
    BOOST_TEST(const_(f)(element1, const_(element2)) == fusion::invoke_function_object<fobj const>(const_(f), const_(seq)));
}

template <class Sequence>
void test_sequence_n(Sequence & seq, mpl::int_<3>)
{
    fobj f;

    BOOST_TEST(f(element1, element2, element3) == fusion::invoke_function_object(f, seq));
    BOOST_TEST(const_(f)(element1, element2, element3) == fusion::invoke_function_object<fobj const>(const_(f), seq));
}

template <class Sequence>
void test_sequence(Sequence & seq)
{
    test_sequence_n(seq, mpl::int_<boost::fusion::result_of::size<Sequence>::value>());
}

void result_type_tests()
{
    using boost::is_same;

    BOOST_TEST(( is_same< boost::fusion::result_of::invoke_function_object< nullary_fobj, fusion::vector<> >::type, int >::value ));
    BOOST_TEST(( is_same< boost::fusion::result_of::invoke_function_object< fobj, fusion::vector<element1_type> >::type, int >::value ));
    BOOST_TEST(( is_same< boost::fusion::result_of::invoke_function_object< fobj, fusion::vector<element1_type,element2_type> >::type, int >::value ));
}


int main()
{
    result_type_tests();

    typedef fusion::vector<> vector0;
    typedef fusion::vector<element1_type> vector1;
    typedef fusion::vector<element1_type, element2_type> vector2;
    typedef fusion::vector<element1_type, element2_type, element3_type> vector3;

    vector0 v0;
    vector1 v1(element1);
    vector2 v2(element1, element2);
    vector3 v3(element1, element2, element3);

    test_sequence(v0);
    test_sequence(v1);
    test_sequence(v2);
    test_sequence(v3);

    typedef fusion::list<> list0;
    typedef fusion::list<element1_type> list1;
    typedef fusion::list<element1_type, element2_type> list2;
    typedef fusion::list<element1_type, element2_type, element3_type> list3;

    list0 l0;
    list1 l1(element1);
    list2 l2(element1, element2);
    list3 l3(element1, element2, element3);

    test_sequence(l0);
    test_sequence(l1);
    test_sequence(l2);
    test_sequence(l3);

    return boost::report_errors();
}

