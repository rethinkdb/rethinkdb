// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: rebind_any.hpp 81428 2012-11-19 21:19:34Z steven_watanabe $

#ifndef BOOST_TYPE_ERASURE_REBIND_ANY_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_REBIND_ANY_HPP_INCLUDED

#include <boost/mpl/if.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_erasure/is_placeholder.hpp>
#include <boost/type_erasure/concept_of.hpp>

namespace boost {
namespace type_erasure {

template<class Concept, class T>
class any;

/**
 * A metafunction that changes the @ref placeholder of
 * an @ref any.  If @c T is not a placeholder,
 * returns @c T unchanged.  This class is intended
 * to be used in @ref concept_interface to deduce
 * the argument types from the arguments of the concept.
 *
 * @pre Any must be a specialization of @ref any or a base
 *      class of such a specialization.
 *
 * \code
 * rebind_any<any<Concept>, _a>::type -> any<Concept, _a>
 * rebind_any<any<Concept>, _b&>::type -> any<Concept, _b&>
 * rebind_any<any<Concept>, int>::type -> int
 * \endcode
 *
 * @see derived, as_param
 */
template<class Any, class T>
struct rebind_any
{
#ifdef BOOST_TYPE_ERASURE_DOXYGEN
    typedef detail::unspecified type;
#else
    typedef typename ::boost::mpl::if_<
        ::boost::type_erasure::is_placeholder<
            typename ::boost::remove_cv<
                typename ::boost::remove_reference<T>::type
            >::type
        >,
        ::boost::type_erasure::any<
            typename ::boost::type_erasure::concept_of<Any>::type,
            T
        >,
        T
    >::type type;
#endif
};

}
}

#endif
