
// NO INCLUDE GUARDS, THE HEADER IS INTENDED FOR MULTIPLE INCLUSION

// Copyright Aleksey Gurtovoy 2002-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// $Source$
// $Date: 2011-10-09 15:28:33 -0700 (Sun, 09 Oct 2011) $
// $Revision: 74865 $

#include <boost/type_traits/detail/template_arity_spec.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/aux_/lambda_support.hpp>
#include <boost/config.hpp>

//
// Unfortunately some libraries have started using this header without
// cleaning up afterwards: so we'd better undef the macros just in case 
// they've been defined already....
//
#ifdef BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL
#undef BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL
#undef BOOST_TT_AUX_BOOL_C_BASE
#undef BOOST_TT_AUX_BOOL_TRAIT_DEF1
#undef BOOST_TT_AUX_BOOL_TRAIT_DEF2
#undef BOOST_TT_AUX_BOOL_TRAIT_SPEC1
#undef BOOST_TT_AUX_BOOL_TRAIT_SPEC2
#undef BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC1
#undef BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC2
#undef BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC1_1
#undef BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC1_2
#undef BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC2_1
#undef BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC2_2
#undef BOOST_TT_AUX_BOOL_TRAIT_IMPL_PARTIAL_SPEC2_1
#undef BOOST_TT_AUX_BOOL_TRAIT_CV_SPEC1
#endif

#if defined(__SUNPRO_CC) && (__SUNPRO_CC < 0x570)
#   define BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
    typedef ::boost::integral_constant<bool,C> type; \
    enum { value = type::value }; \
    /**/
#   define BOOST_TT_AUX_BOOL_C_BASE(C)

#elif defined(BOOST_MSVC) && BOOST_MSVC < 1300

#   define BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
    typedef ::boost::integral_constant<bool,C> base_; \
    using base_::value; \
    /**/

#endif

#ifndef BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL
#   define BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) /**/
#endif

#ifndef BOOST_TT_AUX_BOOL_C_BASE
#   define BOOST_TT_AUX_BOOL_C_BASE(C) : public ::boost::integral_constant<bool,C>
#endif 


#define BOOST_TT_AUX_BOOL_TRAIT_DEF1(trait,T,C) \
template< typename T > struct trait \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
    BOOST_MPL_AUX_LAMBDA_SUPPORT(1,trait,(T)) \
}; \
\
BOOST_TT_AUX_TEMPLATE_ARITY_SPEC(1,trait) \
/**/


#define BOOST_TT_AUX_BOOL_TRAIT_DEF2(trait,T1,T2,C) \
template< typename T1, typename T2 > struct trait \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
    BOOST_MPL_AUX_LAMBDA_SUPPORT(2,trait,(T1,T2)) \
}; \
\
BOOST_TT_AUX_TEMPLATE_ARITY_SPEC(2,trait) \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_DEF3(trait,T1,T2,T3,C) \
template< typename T1, typename T2, typename T3 > struct trait \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
    BOOST_MPL_AUX_LAMBDA_SUPPORT(3,trait,(T1,T2,T3)) \
}; \
\
BOOST_TT_AUX_TEMPLATE_ARITY_SPEC(3,trait) \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_SPEC1(trait,sp,C) \
template<> struct trait< sp > \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
    BOOST_MPL_AUX_LAMBDA_SUPPORT_SPEC(1,trait,(sp)) \
}; \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_SPEC2(trait,sp1,sp2,C) \
template<> struct trait< sp1,sp2 > \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
    BOOST_MPL_AUX_LAMBDA_SUPPORT_SPEC(2,trait,(sp1,sp2)) \
}; \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC1(trait,sp,C) \
template<> struct trait##_impl< sp > \
{ \
public:\
    BOOST_STATIC_CONSTANT(bool, value = (C)); \
}; \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_IMPL_SPEC2(trait,sp1,sp2,C) \
template<> struct trait##_impl< sp1,sp2 > \
{ \
public:\
    BOOST_STATIC_CONSTANT(bool, value = (C)); \
}; \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC1_1(param,trait,sp,C) \
template< param > struct trait< sp > \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
}; \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC1_2(param1,param2,trait,sp,C) \
template< param1, param2 > struct trait< sp > \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
}; \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC2_1(param,trait,sp1,sp2,C) \
template< param > struct trait< sp1,sp2 > \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
    BOOST_MPL_AUX_LAMBDA_SUPPORT_SPEC(2,trait,(sp1,sp2)) \
}; \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC2_2(param1,param2,trait,sp1,sp2,C) \
template< param1, param2 > struct trait< sp1,sp2 > \
    BOOST_TT_AUX_BOOL_C_BASE(C) \
{ \
public:\
    BOOST_TT_AUX_BOOL_TRAIT_VALUE_DECL(C) \
}; \
/**/

#define BOOST_TT_AUX_BOOL_TRAIT_IMPL_PARTIAL_SPEC2_1(param,trait,sp1,sp2,C) \
template< param > struct trait##_impl< sp1,sp2 > \
{ \
public:\
    BOOST_STATIC_CONSTANT(bool, value = (C)); \
}; \
/**/

#ifndef BOOST_NO_CV_SPECIALIZATIONS
#   define BOOST_TT_AUX_BOOL_TRAIT_CV_SPEC1(trait,sp,value) \
    BOOST_TT_AUX_BOOL_TRAIT_SPEC1(trait,sp,value) \
    BOOST_TT_AUX_BOOL_TRAIT_SPEC1(trait,sp const,value) \
    BOOST_TT_AUX_BOOL_TRAIT_SPEC1(trait,sp volatile,value) \
    BOOST_TT_AUX_BOOL_TRAIT_SPEC1(trait,sp const volatile,value) \
    /**/
#else
#   define BOOST_TT_AUX_BOOL_TRAIT_CV_SPEC1(trait,sp,value) \
    BOOST_TT_AUX_BOOL_TRAIT_SPEC1(trait,sp,value) \
    /**/
#endif
