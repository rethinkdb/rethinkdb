
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_OLD_DETAIL_COROUTINE_CALLER_H
#define BOOST_COROUTINES_OLD_DETAIL_COROUTINE_CALLER_H

#include <boost/config.hpp>
#include <boost/context/fcontext.hpp>

#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/v1/detail/coroutine_base.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {
namespace detail {

template< typename Signature, typename Allocator >
class coroutine_caller : public  coroutine_base< Signature >
{
public:
    typedef typename Allocator::template rebind<
        coroutine_caller< Signature, Allocator >
    >::other   allocator_t;

    coroutine_caller( coroutine_context const& callee, bool unwind, bool preserve_fpu,
                    allocator_t const& alloc) BOOST_NOEXCEPT :
        coroutine_base< Signature >( callee, unwind, preserve_fpu),
        alloc_( alloc)
    {}

    void deallocate_object()
    { destroy_( alloc_, this); }

private:
    allocator_t   alloc_;

    static void destroy_( allocator_t & alloc, coroutine_caller * p)
    {
        alloc.destroy( p);
        alloc.deallocate( p, 1);
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_OLD_DETAIL_COROUTINE_CALLER_H
