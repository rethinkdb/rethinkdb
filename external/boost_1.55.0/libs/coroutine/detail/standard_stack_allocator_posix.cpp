
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_COROUTINES_SOURCE

#include "boost/coroutine/detail/standard_stack_allocator.hpp"

extern "C" {
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
}

//#if _POSIX_C_SOURCE >= 200112L

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

#include <boost/assert.hpp>
#include <boost/context/fcontext.hpp>

#include <boost/coroutine/stack_context.hpp>

#if !defined (SIGSTKSZ)
# define SIGSTKSZ (8 * 1024)
# define UDEF_SIGSTKSZ
#endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {
namespace detail {

std::size_t pagesize()
{
    // conform to POSIX.1-2001
    static std::size_t size = ::sysconf( _SC_PAGESIZE);
    return size;
}

rlimit stacksize_limit_()
{
    rlimit limit;
    // conforming to POSIX.1-2001
#if defined(BOOST_DISABLE_ASSERTS)
    ::getrlimit( RLIMIT_STACK, & limit);
#else
    const int result = ::getrlimit( RLIMIT_STACK, & limit);
    BOOST_ASSERT( 0 == result);
#endif
    return limit;
}

rlimit stacksize_limit()
{
    static rlimit limit = stacksize_limit_();
    return limit;
}

std::size_t page_count( std::size_t stacksize)
{
    return static_cast< std::size_t >( 
        std::ceil(
            static_cast< float >( stacksize) / pagesize() ) );
}

bool
standard_stack_allocator::is_stack_unbound()
{ return RLIM_INFINITY == detail::stacksize_limit().rlim_max; }

std::size_t
standard_stack_allocator::default_stacksize()
{
    std::size_t size = 8 * minimum_stacksize();
    if ( is_stack_unbound() ) return size;

    BOOST_ASSERT( maximum_stacksize() >= minimum_stacksize() );
    return maximum_stacksize() == size
        ? size
        : (std::min)( size, maximum_stacksize() );
}

std::size_t
standard_stack_allocator::minimum_stacksize()
{ return SIGSTKSZ + sizeof( context::fcontext_t) + 15; }

std::size_t
standard_stack_allocator::maximum_stacksize()
{
    BOOST_ASSERT( ! is_stack_unbound() );
    return static_cast< std::size_t >( detail::stacksize_limit().rlim_max);
}

void
standard_stack_allocator::allocate( stack_context & ctx, std::size_t size)
{
    BOOST_ASSERT( minimum_stacksize() <= size);
    BOOST_ASSERT( is_stack_unbound() || ( maximum_stacksize() >= size) );

    const std::size_t pages( detail::page_count( size) + 1); // add one guard page
    const std::size_t size_( pages * detail::pagesize() );
    BOOST_ASSERT( 0 < size && 0 < size_);

    const int fd( ::open("/dev/zero", O_RDONLY) );
    BOOST_ASSERT( -1 != fd);
    // conform to POSIX.4 (POSIX.1b-1993, _POSIX_C_SOURCE=199309L)
    void * limit =
# if defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
    ::mmap( 0, size_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
# else
    ::mmap( 0, size_, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
# endif
    ::close( fd);
    if ( ! limit) throw std::bad_alloc();

    std::memset( limit, '\0', size_);

    // conforming to POSIX.1-2001
#if defined(BOOST_DISABLE_ASSERTS)
    ::mprotect( limit, detail::pagesize(), PROT_NONE);
#else
    const int result( ::mprotect( limit, detail::pagesize(), PROT_NONE) );
    BOOST_ASSERT( 0 == result);
#endif

    ctx.size = size_;
    ctx.sp = static_cast< char * >( limit) + ctx.size;
}

void
standard_stack_allocator::deallocate( stack_context & ctx)
{
    BOOST_ASSERT( ctx.sp);
    BOOST_ASSERT( minimum_stacksize() <= ctx.size);
    BOOST_ASSERT( is_stack_unbound() || ( maximum_stacksize() >= ctx.size) );

    void * limit = static_cast< char * >( ctx.sp) - ctx.size;
    // conform to POSIX.4 (POSIX.1b-1993, _POSIX_C_SOURCE=199309L)
    ::munmap( limit, ctx.size);
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#ifdef UDEF_SIGSTKSZ
# undef SIGSTKSZ
#endif
