/*=============================================================================
    Copyright (c) 2005-2006 Joao Abecasis
    Copyright (c) 2006-2007 Tobias Schwinger

    Use modification and distribution are subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#include <boost/fusion/functional/invocation/invoke.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <memory>
#include <boost/noncopyable.hpp>

#include <boost/type_traits/is_same.hpp>

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
    template <typename Sig>
    struct result;

    template <class Self, typename T0>
    struct result< Self(T0)>
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

int nullary() { return 16; }
int unary(int i) { return 17 + i; }
int binary1(int i, object &) { return 18 + i; }
int binary2(int i, object const &) { return 19 + i; }

typedef int (*                   func_ptr)(int);
typedef int (* const           c_func_ptr)(int);
typedef int (* volatile        v_func_ptr)(int);
typedef int (* const volatile cv_func_ptr)(int);

   func_ptr func_ptr1 = &unary;
 c_func_ptr func_ptr2 = &unary;
 v_func_ptr func_ptr3 = &unary;
cv_func_ptr func_ptr4 = &unary;

class members
{
  public:
    int data;

    members()
        : data(20)
    { }

    int nullary() { return data + 1; }
    int nullary_c() const { return data + 2; }
    int unary(int i) { return data + 3 + i; }
    int unary_c(int i) const { return data + 4 + i; }
    int binary(int i, object) { return data + 5 + i; }
    int binary_c(int i, object) const { return data + 6 + i; }
};

struct derived
    : members
{
};

typedef int         element1_type;
typedef object      element2_type;
typedef object_nc & element3_type;

int         element1 = 100;
object      element2 = object();
object_nc   element3;

members that;

std::auto_ptr<members> spt_that(new members);
std::auto_ptr<members const> spt_that_c(new members);

fusion::single_view<members  > sv_obj_ctx(  that);
fusion::single_view<members &> sv_ref_ctx(  that);
fusion::single_view<members *> sv_ptr_ctx(& that);
fusion::single_view<members const  > sv_obj_c_ctx(  that);
fusion::single_view<members const &> sv_ref_c_ctx(  that);
fusion::single_view<members const *> sv_ptr_c_ctx(& that);
fusion::single_view<std::auto_ptr<members> const &> sv_spt_ctx(spt_that);
fusion::single_view< std::auto_ptr<members const> const &> sv_spt_c_ctx(spt_that_c);

derived derived_that;

std::auto_ptr<derived> spt_derived_that(new derived);
std::auto_ptr<derived const> spt_derived_that_c(new derived);

fusion::single_view<derived  > sv_obj_d_ctx(  derived_that);
fusion::single_view<derived &> sv_ref_d_ctx(  derived_that);
fusion::single_view<derived *> sv_ptr_d_ctx(& derived_that);
fusion::single_view<derived const  > sv_obj_c_d_ctx(  derived_that);
fusion::single_view<derived const &> sv_ref_c_d_ctx(  derived_that);
fusion::single_view<derived const *> sv_ptr_c_d_ctx(& derived_that);
fusion::single_view<std::auto_ptr<derived> const &> sv_spt_d_ctx(spt_derived_that);
fusion::single_view< std::auto_ptr<derived const> const &> sv_spt_c_d_ctx(spt_derived_that_c);

template <class Sequence>
void test_sequence_n(Sequence & seq, mpl::int_<0>)
{
    // Function Objects

    nullary_fobj f;

    BOOST_TEST(f () == fusion::invoke(f ,        seq ));
    BOOST_TEST(f () == fusion::invoke(f , const_(seq)));

    // Note: The function object is taken by value, so we request the copy
    // to be const with an explicit template argument. We can also request
    // the function object to be pased by reference...
    BOOST_TEST(const_(f)() == fusion::invoke<nullary_fobj const  >(const_(f),        seq ));
    BOOST_TEST(const_(f)() == fusion::invoke<nullary_fobj const &>(const_(f), const_(seq)));

    nullary_fobj_nc nc_f;
    // ...and we further ensure there is no copying in this case, using a
    // noncopyable function object.
    BOOST_TEST(nc_f () == fusion::invoke<nullary_fobj_nc &>(nc_f ,        seq ));
    BOOST_TEST(nc_f () == fusion::invoke<nullary_fobj_nc &>(nc_f , const_(seq)));
    BOOST_TEST(const_(nc_f)() == fusion::invoke<nullary_fobj_nc const &>(const_(nc_f),        seq ));
    BOOST_TEST(const_(nc_f)() == fusion::invoke<nullary_fobj_nc const &>(const_(nc_f), const_(seq)));

    // Builtin Functions

    // Call through ref/ptr to function
    BOOST_TEST(nullary() == fusion::invoke<int (&)()>(nullary, seq));
    BOOST_TEST(nullary() == fusion::invoke(& nullary, seq));

    // Call through ptr to member function
    // Note: The non-const function members::nullary can't be invoked with
    // fusion::join(sv_obj_ctx,seq)), which is const and so is its first element
    BOOST_TEST(that.nullary() == fusion::invoke(& members::nullary, fusion::join(sv_ref_ctx,seq)));
    BOOST_TEST(that.nullary() == fusion::invoke(& members::nullary, fusion::join(sv_ptr_ctx,seq)));
    BOOST_TEST(that.nullary() == fusion::invoke(& members::nullary, fusion::join(sv_spt_ctx,seq)));
    BOOST_TEST(that.nullary_c() == fusion::invoke(& members::nullary_c, fusion::join(sv_obj_ctx,seq)));
    BOOST_TEST(that.nullary_c() == fusion::invoke(& members::nullary_c, fusion::join(sv_ref_ctx,seq)));
    BOOST_TEST(that.nullary_c() == fusion::invoke(& members::nullary_c, fusion::join(sv_ptr_ctx,seq)));
    BOOST_TEST(that.nullary_c() == fusion::invoke(& members::nullary_c, fusion::join(sv_spt_ctx,seq)));
    BOOST_TEST(that.nullary_c() == fusion::invoke(& members::nullary_c, fusion::join(sv_obj_c_ctx,seq)));
    BOOST_TEST(that.nullary_c() == fusion::invoke(& members::nullary_c, fusion::join(sv_ref_c_ctx,seq)));
    BOOST_TEST(that.nullary_c() == fusion::invoke(& members::nullary_c, fusion::join(sv_ptr_c_ctx,seq)));
    BOOST_TEST(that.nullary_c() == fusion::invoke(& members::nullary_c, fusion::join(sv_spt_c_ctx,seq)));

    // Pointer to data member

    BOOST_TEST(that.data == (fusion::invoke(& members::data, fusion::join(sv_obj_ctx,seq)) = that.data));
    BOOST_TEST(that.data == (fusion::invoke(& members::data, fusion::join(sv_ref_ctx,seq)) = that.data));
    BOOST_TEST(that.data == (fusion::invoke(& members::data, fusion::join(sv_ptr_ctx,seq)) = that.data));
    BOOST_TEST(that.data == (fusion::invoke(& members::data, fusion::join(sv_spt_ctx,seq)) = that.data));
    BOOST_TEST(that.data == fusion::invoke(& members::data, fusion::join(sv_obj_c_ctx,seq)));
    BOOST_TEST(that.data == fusion::invoke(& members::data, fusion::join(sv_ref_c_ctx,seq)));
    BOOST_TEST(that.data == fusion::invoke(& members::data, fusion::join(sv_ptr_c_ctx,seq)));
    BOOST_TEST(that.data == fusion::invoke(& members::data, fusion::join(sv_spt_c_ctx,seq)));

    BOOST_TEST(that.data == (fusion::invoke(& members::data, fusion::join(sv_obj_d_ctx,seq)) = that.data));
    BOOST_TEST(that.data == (fusion::invoke(& members::data, fusion::join(sv_ref_d_ctx,seq)) = that.data));
    BOOST_TEST(that.data == (fusion::invoke(& members::data, fusion::join(sv_ptr_d_ctx,seq)) = that.data));
    BOOST_TEST(that.data == (fusion::invoke(& members::data, fusion::join(sv_spt_d_ctx,seq)) = that.data));
    BOOST_TEST(that.data == fusion::invoke(& members::data, fusion::join(sv_obj_c_d_ctx,seq)));
    BOOST_TEST(that.data == fusion::invoke(& members::data, fusion::join(sv_ref_c_d_ctx,seq)));
    BOOST_TEST(that.data == fusion::invoke(& members::data, fusion::join(sv_ptr_c_d_ctx,seq)));
    BOOST_TEST(that.data == fusion::invoke(& members::data, fusion::join(sv_spt_c_d_ctx,seq)));
}

template <class Sequence>
void test_sequence_n(Sequence & seq, mpl::int_<1>)
{
    fobj f;
    BOOST_TEST(f(element1) == fusion::invoke(f , seq ));
    BOOST_TEST(f(element1) == fusion::invoke(f , const_(seq)));
    BOOST_TEST(const_(f)(element1) == fusion::invoke<fobj const  >(const_(f), seq ));
    BOOST_TEST(const_(f)(element1) == fusion::invoke<fobj const &>(const_(f), const_(seq)));

    fobj_nc nc_f;
    BOOST_TEST(nc_f(element1) == fusion::invoke<fobj_nc &>(nc_f, seq ));
    BOOST_TEST(nc_f(element1) == fusion::invoke<fobj_nc &>(nc_f, const_(seq)));
    BOOST_TEST(const_(nc_f)(element1) == fusion::invoke<fobj_nc const &>(const_(nc_f), seq ));
    BOOST_TEST(const_(nc_f)(element1) == fusion::invoke<fobj_nc const &>(const_(nc_f), const_(seq)));

    BOOST_TEST(unary(element1) == fusion::invoke<int (&)(int)>(unary, seq));
    BOOST_TEST(func_ptr1(element1) == fusion::invoke(func_ptr1, seq));
    BOOST_TEST(func_ptr2(element1) == fusion::invoke(func_ptr2, seq));
    BOOST_TEST(func_ptr3(element1) == fusion::invoke(func_ptr3, seq));
    BOOST_TEST(func_ptr4(element1) == fusion::invoke(func_ptr4, seq));

    BOOST_TEST(that.unary(element1) == fusion::invoke(& members::unary, fusion::join(sv_ref_ctx,seq)));
    BOOST_TEST(that.unary(element1) == fusion::invoke(& members::unary, fusion::join(sv_ptr_ctx,seq)));
    BOOST_TEST(that.unary(element1) == fusion::invoke(& members::unary, fusion::join(sv_spt_ctx,seq)));
    BOOST_TEST(that.unary_c(element1) == fusion::invoke(& members::unary_c, fusion::join(sv_obj_ctx,seq)));
    BOOST_TEST(that.unary_c(element1) == fusion::invoke(& members::unary_c, fusion::join(sv_ref_ctx,seq)));
    BOOST_TEST(that.unary_c(element1) == fusion::invoke(& members::unary_c, fusion::join(sv_ptr_ctx,seq)));
    BOOST_TEST(that.unary_c(element1) == fusion::invoke(& members::unary_c, fusion::join(sv_spt_ctx,seq)));
    BOOST_TEST(that.unary_c(element1) == fusion::invoke(& members::unary_c, fusion::join(sv_obj_c_ctx,seq)));
    BOOST_TEST(that.unary_c(element1) == fusion::invoke(& members::unary_c, fusion::join(sv_ref_c_ctx,seq)));
    BOOST_TEST(that.unary_c(element1) == fusion::invoke(& members::unary_c, fusion::join(sv_ptr_c_ctx,seq)));
    BOOST_TEST(that.unary_c(element1) == fusion::invoke(& members::unary_c, fusion::join(sv_spt_c_ctx,seq)));

    BOOST_TEST(that.unary(element1) == fusion::invoke(& members::unary, fusion::join(sv_ref_d_ctx,seq)));
    BOOST_TEST(that.unary(element1) == fusion::invoke(& members::unary, fusion::join(sv_ptr_d_ctx,seq)));
    BOOST_TEST(that.unary(element1) == fusion::invoke(& members::unary, fusion::join(sv_spt_d_ctx,seq)));
    BOOST_TEST(that.unary_c(element1) == fusion::invoke(& members::unary_c, fusion::join(sv_obj_d_ctx,seq)));
    BOOST_TEST(that.unary_c(element1) == fusion::invoke(& members::unary_c, fusion::join(sv_ref_d_ctx,seq)));
    BOOST_TEST(that.unary_c(element1) == fusion::invoke(& members::unary_c, fusion::join(sv_ptr_d_ctx,seq)));
    BOOST_TEST(that.unary_c(element1) == fusion::invoke(& members::unary_c, fusion::join(sv_spt_d_ctx,seq)));
    BOOST_TEST(that.unary_c(element1) == fusion::invoke(& members::unary_c, fusion::join(sv_obj_c_d_ctx,seq)));
    BOOST_TEST(that.unary_c(element1) == fusion::invoke(& members::unary_c, fusion::join(sv_ref_c_d_ctx,seq)));
    BOOST_TEST(that.unary_c(element1) == fusion::invoke(& members::unary_c, fusion::join(sv_ptr_c_d_ctx,seq)));
    BOOST_TEST(that.unary_c(element1) == fusion::invoke(& members::unary_c, fusion::join(sv_spt_c_d_ctx,seq)));

}

template <class Sequence>
void test_sequence_n(Sequence & seq, mpl::int_<2>)
{
    fobj f;
    BOOST_TEST(f (element1, element2) == fusion::invoke(f , seq));
    BOOST_TEST(f (element1, const_(element2)) == fusion::invoke(f , const_(seq)));
    BOOST_TEST(const_(f)(element1, element2) == fusion::invoke<fobj const>(const_(f), seq));
    BOOST_TEST(const_(f)(element1, const_(element2)) == fusion::invoke<fobj const>(const_(f), const_(seq)));

    BOOST_TEST(binary1(element1, element2) == fusion::invoke(binary1, seq));
    BOOST_TEST(binary2(element1, element2) == fusion::invoke(binary2, seq));

    BOOST_TEST(that.binary(element1,element2) == fusion::invoke(& members::binary, fusion::join(sv_ref_ctx,seq)));
    BOOST_TEST(that.binary(element1,element2) == fusion::invoke(& members::binary, fusion::join(sv_ptr_ctx,seq)));
    BOOST_TEST(that.binary(element1,element2) == fusion::invoke(& members::binary, fusion::join(sv_spt_ctx,seq)));
    BOOST_TEST(that.binary_c(element1,element2) == fusion::invoke(& members::binary_c, fusion::join(sv_obj_ctx,seq)));
    BOOST_TEST(that.binary_c(element1,element2) == fusion::invoke(& members::binary_c, fusion::join(sv_ref_ctx,seq)));
    BOOST_TEST(that.binary_c(element1,element2) == fusion::invoke(& members::binary_c, fusion::join(sv_ptr_ctx,seq)));
    BOOST_TEST(that.binary_c(element1,element2) == fusion::invoke(& members::binary_c, fusion::join(sv_spt_ctx,seq)));
    BOOST_TEST(that.binary_c(element1,element2) == fusion::invoke(& members::binary_c, fusion::join(sv_obj_c_ctx,seq)));
    BOOST_TEST(that.binary_c(element1,element2) == fusion::invoke(& members::binary_c, fusion::join(sv_ref_c_ctx,seq)));
    BOOST_TEST(that.binary_c(element1,element2) == fusion::invoke(& members::binary_c, fusion::join(sv_ptr_c_ctx,seq)));
    BOOST_TEST(that.binary_c(element1,element2) == fusion::invoke(& members::binary_c, fusion::join(sv_spt_c_ctx,seq)));
}

template <class Sequence>
void test_sequence_n(Sequence & seq, mpl::int_<3>)
{
    fobj f;

    BOOST_TEST(f(element1, element2, element3) == fusion::invoke(f, seq));
    BOOST_TEST(const_(f)(element1, element2, element3) == fusion::invoke<fobj const>(const_(f), seq));
}

template <class Sequence>
void test_sequence(Sequence & seq)
{
    test_sequence_n(seq, mpl::int_<boost::fusion::result_of::size<Sequence>::value>());
}


void result_type_tests()
{
    using boost::is_same;

    BOOST_TEST(( is_same<
      boost::fusion::result_of::invoke<int (*)(), fusion::vector0<> >::type, int
    >::value ));
// disabled until boost::result_of supports it
//    BOOST_TEST(( is_same<
//      boost::fusion::result_of::invoke<int (*)(...), fusion::vector1<int> >::type, int
//    >::value ));
    BOOST_TEST(( is_same<
      boost::fusion::result_of::invoke<int (members::*)(), fusion::vector1<members*> >::type, int
    >::value ));
// disabled until boost::result_of supports it
//    BOOST_TEST(( is_same<
//      boost::fusion::result_of::invoke<int (members::*)(...), fusion::vector2<members*,int> >::type, int
//    >::value ));
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

