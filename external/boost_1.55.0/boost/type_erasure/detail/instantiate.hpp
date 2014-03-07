// Boost.TypeErasure library
//
// Copyright 2012 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: instantiate.hpp 80881 2012-10-05 23:36:21Z steven_watanabe $

#if !defined(BOOST_PP_IS_ITERATING)

#ifndef BOOST_TYPE_ERASURE_DETAIL_INSTANTIATE_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_INSTANTIATE_HPP_INCLUDED

#include <boost/mpl/transform.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/at.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/type_erasure/detail/normalize.hpp>
#include <boost/type_erasure/detail/rebind_placeholders.hpp>

namespace boost {
namespace type_erasure {
namespace detail {

template<int N>
struct make_instantiate_concept_impl;

template<class Concept>
struct make_instantiate_concept {
    typedef typename ::boost::type_erasure::detail::normalize_concept<
        Concept>::type normalized;
    typedef typename ::boost::type_erasure::detail::make_instantiate_concept_impl<
        (::boost::mpl::size<normalized>::value)
    >::type type;
};

#define BOOST_TYPE_ERASURE_INSTANTIATE(Concept, Map)            \
    (::boost::type_erasure::detail::make_instantiate_concept<   \
        Concept                                                 \
    >::type::apply((Concept*)0, (Map*)0))

#define BOOST_TYPE_ERASURE_INSTANTIATE1(Concept, P0, T0)        \
    (::boost::type_erasure::detail::make_instantiate_concept<   \
        Concept                                                 \
    >::type::apply(                                             \
        (Concept*)0,                                            \
        (::boost::mpl::map1< ::boost::mpl::pair<P0, T0> >*)0))

#define BOOST_PP_FILENAME_1 <boost/type_erasure/detail/instantiate.hpp>
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_TYPE_ERASURE_MAX_FUNCTIONS)
#include BOOST_PP_ITERATE()

}
}
}

#endif

#else

#define N BOOST_PP_ITERATION()

#define BOOST_TYPE_ERASURE_INSTANTIATE_IMPL(z, n, data)\
    (void)&::boost::mpl::at_c<data, n>::type::apply;

struct BOOST_PP_CAT(instantiate_concept, N) {
    template<class Concept, class Map>
    static void apply(Concept *, Map *) {
        typedef typename ::boost::type_erasure::detail::normalize_concept<
            Concept>::type normalized;
        typedef typename ::boost::type_erasure::detail::get_placeholder_normalization_map<
            Concept
        >::type placeholder_subs;

        typedef typename ::boost::mpl::transform<
            normalized,
            ::boost::type_erasure::detail::rebind_placeholders<
                ::boost::mpl::_1,
                typename ::boost::type_erasure::detail::add_deductions<
                    Map,
                    placeholder_subs
                >::type
            >
        >::type concept_sequence;
        BOOST_PP_REPEAT(N, BOOST_TYPE_ERASURE_INSTANTIATE_IMPL, concept_sequence)
    }
};

template<>
struct make_instantiate_concept_impl<N>
{
    typedef ::boost::type_erasure::detail::BOOST_PP_CAT(instantiate_concept, N) type;
};

#undef BOOST_TYPE_ERASURE_INSTANTIATE_IMPL

#undef N

#endif
