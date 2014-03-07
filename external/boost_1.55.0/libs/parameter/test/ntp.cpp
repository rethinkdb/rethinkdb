// Copyright Daniel Wallin 2006. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/parameter.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>

namespace mpl = boost::mpl;
namespace parameter = boost::parameter;

template <class T = int>
struct a0_is
  : parameter::template_keyword<a0_is<>, T>
{};

template <class T = int>
struct a1_is
  : parameter::template_keyword<a1_is<>, T>
{};

template <class T = int>
struct a2_is
  : parameter::template_keyword<a2_is<>, T>
{};

template <class T = int>
struct a3_is
  : parameter::template_keyword<a3_is<>, T>
{};

struct X {};
struct Y : X {};

template <
    class A0 = parameter::void_
  , class A1 = parameter::void_
  , class A2 = parameter::void_
  , class A3 = parameter::void_
>
struct with_ntp
{
    typedef typename parameter::parameters<
        a0_is<>, a1_is<>, a2_is<>
      , parameter::optional<
            parameter::deduced<a3_is<> >
          , boost::is_base_and_derived<X, mpl::_>
        >
    >::bind<A0,A1,A2,A3
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
          , parameter::void_
#endif 
            >::type args;

    typedef typename parameter::binding<
        args, a0_is<>, void*
    >::type a0;

    typedef typename parameter::binding<
        args, a1_is<>, void*
    >::type a1;

    typedef typename parameter::binding<
        args, a2_is<>, void*
    >::type a2;

    typedef typename parameter::binding<
        args, a3_is<>, void*
    >::type a3;

    typedef void(*type)(a0,a1,a2,a3);
};

BOOST_MPL_ASSERT((boost::is_same<
    with_ntp<>::type, void(*)(void*,void*,void*,void*)
>));

BOOST_MPL_ASSERT((boost::is_same<
    with_ntp<a2_is<int> >::type, void(*)(void*,void*,int,void*)
>));

BOOST_MPL_ASSERT((boost::is_same<
    with_ntp<a1_is<int> >::type, void(*)(void*,int,void*,void*)
>));

BOOST_MPL_ASSERT((boost::is_same<
    with_ntp<a2_is<int const>, a1_is<float> >::type, void(*)(void*,float,int const,void*)
>));

BOOST_MPL_ASSERT((boost::is_same<
    with_ntp<int const>::type, void(*)(int const, void*, void*,void*)
>));

BOOST_MPL_ASSERT((boost::is_same<
    with_ntp<int, float>::type, void(*)(int, float, void*,void*)
>));

BOOST_MPL_ASSERT((boost::is_same<
    with_ntp<int, float, char>::type, void(*)(int, float, char,void*)
>));

BOOST_MPL_ASSERT((boost::is_same<
    with_ntp<a0_is<int>, Y>::type, void(*)(int,void*,void*, Y)
>));

BOOST_MPL_ASSERT((boost::is_same<
    with_ntp<int&, a2_is<char>, Y>::type, void(*)(int&,void*,char, Y)
>));

