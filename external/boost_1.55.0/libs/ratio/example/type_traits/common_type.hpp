//  common_type.hpp  ---------------------------------------------------------//

//  Copyright 2008 Howard Hinnant
//  Copyright 2008 Beman Dawes

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_EX_TYPE_TRAITS_EXT_COMMON_TYPE_HPP
#define BOOST_EX_TYPE_TRAITS_EXT_COMMON_TYPE_HPP

#include <boost/config.hpp>

//----------------------------------------------------------------------------//
#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#define BOOST_EX_COMMON_TYPE_ARITY 3
#endif

//----------------------------------------------------------------------------//
#if defined(BOOST_NO_CXX11_DECLTYPE) && !defined(BOOST_EX_COMMON_TYPE_DONT_USE_TYPEOF)
#define BOOST_TYPEOF_SILENT
#include <boost/typeof/typeof.hpp>   // boost wonders never cease!
#endif

//----------------------------------------------------------------------------//
#ifndef BOOST_NO_CXX11_STATIC_ASSERT
#define BOOST_EX_COMMON_TYPE_STATIC_ASSERT(CND, MSG, TYPES) static_assert(CND,MSG)
#elif defined(BOOST_EX_COMMON_TYPE_USES_STATIC_ASSERT)
#include <boost/static_assert.hpp>
#define BOOST_EX_COMMON_TYPE_STATIC_ASSERT(CND, MSG, TYPES) BOOST_STATIC_ASSERT(CND)
#elif defined(BOOST_EX_COMMON_TYPE_USES_MPL_ASSERT)
#include <boost/mpl/assert.hpp>
#include <boost/mpl/bool.hpp>
#define BOOST_EX_COMMON_TYPE_STATIC_ASSERT(CND, MSG, TYPES)                                 \
    BOOST_MPL_ASSERT_MSG(boost::mpl::bool_< (CND) >::type::value, MSG, TYPES)
#else
//~ #elif defined(BOOST_EX_COMMON_TYPE_USES_ARRAY_ASSERT)
#define BOOST_EX_COMMON_TYPE_CONCAT(A,B) A##B
#define BOOST_EX_COMMON_TYPE_NAME(A,B) BOOST_EX_COMMON_TYPE_CONCAT(A,B)
#define BOOST_EX_COMMON_TYPE_STATIC_ASSERT(CND, MSG, TYPES) static char BOOST_EX_COMMON_TYPE_NAME(__boost_common_type_test_,__LINE__)[(CND)?1:-1]
//~ #define BOOST_EX_COMMON_TYPE_STATIC_ASSERT(CND, MSG, TYPES)
#endif

#if !defined(BOOST_NO_CXX11_STATIC_ASSERT) || !defined(BOOST_EX_COMMON_TYPE_USES_MPL_ASSERT)
#define BOOST_EX_COMMON_TYPE_MUST_BE_A_COMPLE_TYPE "must be complete type"
#endif

#if defined(BOOST_NO_CXX11_DECLTYPE) && defined(BOOST_EX_COMMON_TYPE_DONT_USE_TYPEOF)
#include "detail/common_type.hpp"
#include <boost/type_traits/remove_cv.hpp>
#endif
#include <boost/mpl/if.hpp>
#include "declval.hpp"

//----------------------------------------------------------------------------//
//                                                                            //
//                           C++03 implementation of                          //
//             20.6.7 Other transformations [meta.trans.other]                //
//                          Written by Howard Hinnant                         //
//      Adapted for Boost by Beman Dawes, Vicente Botet and  Jeffrey Hellrung //
//                                                                            //
//----------------------------------------------------------------------------//

namespace boost_ex {

// prototype
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template<typename... T>
    struct common_type;
#else // or no specialization
    template <class T, class U = void, class V = void>
    struct common_type
    {
    public:
        typedef typename common_type<typename common_type<T, U>::type, V>::type type;
    };
#endif


// 1 arg
    template<typename T>
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    struct common_type<T>
#else
    struct common_type<T, void, void>

#endif
    {
        BOOST_EX_COMMON_TYPE_STATIC_ASSERT(sizeof(T) > 0, BOOST_EX_COMMON_TYPE_MUST_BE_A_COMPLE_TYPE, (T));
    public:
        typedef T type;
    };

// 2 args
namespace type_traits_detail {

    template <class T, class U>
    struct common_type_2
    {
    private:
        BOOST_EX_COMMON_TYPE_STATIC_ASSERT(sizeof(T) > 0, BOOST_EX_COMMON_TYPE_MUST_BE_A_COMPLE_TYPE, (T));
        BOOST_EX_COMMON_TYPE_STATIC_ASSERT(sizeof(U) > 0, BOOST_EX_COMMON_TYPE_MUST_BE_A_COMPLE_TYPE, (U));
        static bool declval_bool();  // workaround gcc bug; not required by std
        static typename add_rvalue_reference<T>::type declval_T();  // workaround gcc bug; not required by std
        static typename add_rvalue_reference<U>::type declval_U();  // workaround gcc bug; not required by std

#if !defined(BOOST_NO_CXX11_DECLTYPE)
    public:
        typedef decltype(declval<bool>() ? declval<T>() : declval<U>()) type;
#elif defined(BOOST_EX_COMMON_TYPE_DONT_USE_TYPEOF)
    public:
    typedef typename detail_type_traits_common_type::common_type_impl<
          typename remove_cv<T>::type,
          typename remove_cv<U>::type
      >::type type;
#else
    public:
        //~ typedef BOOST_TYPEOF_TPL(declval_bool() ? declval_T() : declval_U()) type;
        typedef BOOST_TYPEOF_TPL(declval<bool>() ? declval<T>() : declval<U>()) type;
#endif
    };

    template <class T>
    struct common_type_2<T, T>
    {
        typedef T type;
    };
    }

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template <class T, class U>
    struct common_type<T, U>
#else
    template <class T, class U>
    struct common_type<T, U, void>
#endif
    : type_traits_detail::common_type_2<T,U>
    { };


// 3 or more args
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template<typename T, typename U, typename... V>
    struct common_type<T, U, V...> {
    public:
        typedef typename common_type<typename common_type<T, U>::type, V...>::type type;
    };
#endif
}  // namespace boost_ex

#endif  // BOOST_TYPE_TRAITS_EXT_COMMON_TYPE_HPP
