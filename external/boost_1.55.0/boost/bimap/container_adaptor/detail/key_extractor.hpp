// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file container_adaptor/detail/key_extractor.hpp
/// \brief Key extractor for a pair<Key,Data>.

#ifndef BOOST_BIMAP_CONTAINER_ADAPTOR_DETAIL_KEY_EXTRACTOR_HPP
#define BOOST_BIMAP_CONTAINER_ADAPTOR_DETAIL_KEY_EXTRACTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER>=1200)
#pragma once
#endif

#include <boost/config.hpp>

#include <functional>

namespace boost {
namespace bimaps {
namespace container_adaptor {
namespace detail {

/// \brief Key Extractor

template < class T >
struct key_from_pair_extractor 
    : std::unary_function< T, BOOST_DEDUCED_TYPENAME T::first_type >
{
    bool operator()( const T & p ) { return p.first; }
};

} // namespace detail
} // namespace container_adaptor
} // namespace bimaps
} // namespace boost


#endif // BOOST_BIMAP_CONTAINER_ADAPTOR_DETAIL_KEY_EXTRACTOR_HPP


