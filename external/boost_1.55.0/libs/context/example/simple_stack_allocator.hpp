
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_SIMPLE_STACK_ALLOCATOR_H
#define BOOST_CONTEXT_SIMPLE_STACK_ALLOCATOR_H

#include <cstddef>
#include <cstdlib>
#include <stdexcept>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/context/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace context {

template< std::size_t Max, std::size_t Default, std::size_t Min >
class simple_stack_allocator
{
public:
    static std::size_t maximum_stacksize()
    { return Max; }

    static std::size_t default_stacksize()
    { return Default; }

    static std::size_t minimum_stacksize()
    { return Min; }

    void * allocate( std::size_t size) const
    {
        BOOST_ASSERT( minimum_stacksize() <= size);
        BOOST_ASSERT( maximum_stacksize() >= size);

        void * limit = std::calloc( size, sizeof( char) );
        if ( ! limit) throw std::bad_alloc();

        return static_cast< char * >( limit) + size;
    }

    void deallocate( void * vp, std::size_t size) const
    {
        BOOST_ASSERT( vp);
        BOOST_ASSERT( minimum_stacksize() <= size);
        BOOST_ASSERT( maximum_stacksize() >= size);

        void * limit = static_cast< char * >( vp) - size;
        std::free( limit);
    }
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXT_SIMPLE_STACK_ALLOCATOR_H
