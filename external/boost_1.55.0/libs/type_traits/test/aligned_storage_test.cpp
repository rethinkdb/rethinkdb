
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#  include <boost/type_traits/type_with_alignment.hpp> // max_align and long_long_type
#else
#  include <boost/type_traits/alignment_of.hpp>
#  include <boost/type_traits/aligned_storage.hpp>
#  include <boost/type_traits/is_pod.hpp>
#endif

template <class T>
union must_be_pod
{
   int i;
   T t;
};

template <class T>
inline void no_unused_warning(const volatile T&)
{
}

#if defined(__GNUC__) && (__GNUC__ >= 4) && !defined(BOOST_INTEL)
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif

template <class T>
void do_check(const T&)
{
   typedef typename tt::aligned_storage<T::value,T::value>::type t1;
   t1 as1 = { 0, };
   must_be_pod<t1> pod1;
   no_unused_warning(as1);
   no_unused_warning(pod1);
   BOOST_TEST_MESSAGE(typeid(t1).name());
   BOOST_CHECK(::tt::alignment_of<t1>::value == T::value);
   BOOST_CHECK(sizeof(t1) == T::value);
#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
   BOOST_CHECK(::tt::is_pod<t1>::value == true);
#endif
   typedef typename tt::aligned_storage<T::value*2,T::value>::type t2;
   t2 as2 = { 0, };
   must_be_pod<t2> pod2;
   no_unused_warning(as2);
   no_unused_warning(pod2);
   BOOST_TEST_MESSAGE(typeid(t2).name());
   BOOST_CHECK(::tt::alignment_of<t2>::value == T::value);
   BOOST_CHECK(sizeof(t2) == T::value*2);
#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
   BOOST_CHECK(::tt::is_pod<t2>::value == true);
#endif

#ifndef TEST_STD
   // Non-Tr1 behaviour:
   typedef typename tt::aligned_storage<T::value, ~static_cast<std::size_t>(0UL)>::type t3;
   t3 as3 = { 0, };
   must_be_pod<t3> pod3;
   no_unused_warning(as3);
   no_unused_warning(pod3);
   BOOST_TEST_MESSAGE(typeid(t3).name());
   BOOST_CHECK(::tt::alignment_of<t3>::value == ::tt::alignment_of< ::boost::detail::max_align>::value);
   BOOST_CHECK((sizeof(t3) % T::value) == 0);
#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
   BOOST_CHECK(::tt::is_pod<t3>::value == true);
#endif
   BOOST_CHECK(as3.address() == &as3);
   const t3 as4 = { 0, };
   BOOST_CHECK(as4.address() == static_cast<const void*>(&as4));
#endif
}

TT_TEST_BEGIN(type_with_alignment)

do_check(tt::integral_constant<std::size_t,::tt::alignment_of<char>::value>());
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<short>::value>());
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<int>::value>());
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<long>::value>());
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<float>::value>());
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<double>::value>());
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<long double>::value>());

#ifdef BOOST_HAS_LONG_LONG
do_check(tt::integral_constant<std::size_t,::tt::alignment_of< ::boost::long_long_type>::value>());
#endif
#ifdef BOOST_HAS_MS_INT64
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<__int64>::value>());
#endif
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<int[4]>::value>());
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<int(*)(int)>::value>());
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<int*>::value>());
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<VB>::value>());
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<VD>::value>());
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<enum_UDT>::value>());
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<mf2>::value>());
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<POD_UDT>::value>());
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<empty_UDT>::value>());
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<union_UDT>::value>());
do_check(tt::integral_constant<std::size_t,::tt::alignment_of<boost::detail::max_align>::value>());

TT_TEST_END









