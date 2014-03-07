
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_OLD_DETAIL_COROUTINE_GET_H
#define BOOST_COROUTINES_OLD_DETAIL_COROUTINE_GET_H

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/type_traits/function_traits.hpp>

#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/detail/param.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {
namespace detail {

template<
    typename D,
    typename Result, int arity
>
struct coroutine_get;

template< typename D, int arity >
struct coroutine_get< D, void, arity >
{};

template< typename D, typename Result, int arity >
struct coroutine_get
{
    bool has_result() const
    { return static_cast< D const* >( this)->impl_->result_; }

    typename param< Result >::type get() const
    {
        BOOST_ASSERT( static_cast< D const* >( this)->impl_->result_);
        return static_cast< D const* >( this)->impl_->result_.get();
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_OLD_DETAIL_COROUTINE_GET_H
