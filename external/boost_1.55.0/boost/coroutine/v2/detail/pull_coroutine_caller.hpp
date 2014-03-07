//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_UNIDIRECT_DETAIL_PULL_COROUTINE_CALLER_H
#define BOOST_COROUTINES_UNIDIRECT_DETAIL_PULL_COROUTINE_CALLER_H

#include <boost/config.hpp>
#include <boost/context/fcontext.hpp>
#include <boost/optional.hpp>

#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/v2/detail/pull_coroutine_base.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {
namespace detail {

template< typename R, typename Allocator >
class pull_coroutine_caller : public  pull_coroutine_base< R >
{
public:
    typedef typename Allocator::template rebind<
        pull_coroutine_caller< R, Allocator >
    >::other   allocator_t;

    pull_coroutine_caller( coroutine_context const& callee, bool unwind, bool preserve_fpu,
                           allocator_t const& alloc, optional< R > const& data) BOOST_NOEXCEPT :
        pull_coroutine_base< R >( callee, unwind, preserve_fpu, data),
        alloc_( alloc)
    {}

    void deallocate_object()
    { destroy_( alloc_, this); }

private:
    allocator_t   alloc_;

    static void destroy_( allocator_t & alloc, pull_coroutine_caller * p)
    {
        alloc.destroy( p);
        alloc.deallocate( p, 1);
    }
};

template< typename R, typename Allocator >
class pull_coroutine_caller< R &, Allocator > : public  pull_coroutine_base< R & >
{
public:
    typedef typename Allocator::template rebind<
        pull_coroutine_caller< R &, Allocator >
    >::other   allocator_t;

    pull_coroutine_caller( coroutine_context const& callee, bool unwind, bool preserve_fpu,
                           allocator_t const& alloc, optional< R * > const& data) BOOST_NOEXCEPT :
        pull_coroutine_base< R & >( callee, unwind, preserve_fpu, data),
        alloc_( alloc)
    {}

    void deallocate_object()
    { destroy_( alloc_, this); }

private:
    allocator_t   alloc_;

    static void destroy_( allocator_t & alloc, pull_coroutine_caller * p)
    {
        alloc.destroy( p);
        alloc.deallocate( p, 1);
    }
};

template< typename Allocator >
class pull_coroutine_caller< void, Allocator > : public  pull_coroutine_base< void >
{
public:
    typedef typename Allocator::template rebind<
        pull_coroutine_caller< void, Allocator >
    >::other   allocator_t;

    pull_coroutine_caller( coroutine_context const& callee, bool unwind, bool preserve_fpu,
                           allocator_t const& alloc) BOOST_NOEXCEPT :
        pull_coroutine_base< void >( callee, unwind, preserve_fpu),
        alloc_( alloc)
    {}

    void deallocate_object()
    { destroy_( alloc_, this); }

private:
    allocator_t   alloc_;

    static void destroy_( allocator_t & alloc, pull_coroutine_caller * p)
    {
        alloc.destroy( p);
        alloc.deallocate( p, 1);
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_UNIDIRECT_DETAIL_PULL_COROUTINE_CALLER_H
