
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_FCONTEXT_H
#define BOOST_CONTEXT_FCONTEXT_H

#if defined(__PGI)
#include <stdint.h>
#endif

#if defined(_WIN32_WCE)
typedef int intptr_t;
#endif

#include <boost/config.hpp>
#include <boost/cstdint.hpp>

#include <boost/context/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

// x86_64
// test x86_64 before i386 because icc might
// define __i686__ for x86_64 too
#if defined(__x86_64__) || defined(__x86_64) \
    || defined(__amd64__) || defined(__amd64) \
    || defined(_M_X64) || defined(_M_AMD64)
# if defined(BOOST_WINDOWS)
#  include <boost/context/detail/fcontext_x86_64_win.hpp>
# else
#  include <boost/context/detail/fcontext_x86_64.hpp>
# endif
// i386
#elif defined(i386) || defined(__i386__) || defined(__i386) \
    || defined(__i486__) || defined(__i586__) || defined(__i686__) \
    || defined(__X86__) || defined(_X86_) || defined(__THW_INTEL__) \
    || defined(__I86__) || defined(__INTEL__) || defined(__IA32__) \
    || defined(_M_IX86) || defined(_I86_)
# if defined(BOOST_WINDOWS)
#  include <boost/context/detail/fcontext_i386_win.hpp>
# else
#  include <boost/context/detail/fcontext_i386.hpp>
# endif
// arm
#elif defined(__arm__) || defined(__thumb__) || defined(__TARGET_ARCH_ARM) \
    || defined(__TARGET_ARCH_THUMB) || defined(_ARM) || defined(_M_ARM)
# include <boost/context/detail/fcontext_arm.hpp>
// mips
#elif (defined(__mips) && __mips == 1) || defined(_MIPS_ISA_MIPS1) \
    || defined(_R3000)
# include <boost/context/detail/fcontext_mips.hpp>
// powerpc
#elif defined(__powerpc) || defined(__powerpc__) || defined(__ppc) \
    || defined(__ppc__) || defined(_ARCH_PPC) || defined(__POWERPC__) \
    || defined(__PPCGECKO__) || defined(__PPCBROADWAY) || defined(_XENON)
# include <boost/context/detail/fcontext_ppc.hpp>
#elif defined(__sparc__) || defined(__sparc)
// sparc or sparc64
# include <boost/context/detail/fcontext_sparc.hpp>
#else
# error "platform not supported"
#endif

namespace boost {
namespace context {

extern "C" BOOST_CONTEXT_DECL
intptr_t BOOST_CONTEXT_CALLDECL jump_fcontext( fcontext_t * ofc, fcontext_t const* nfc, intptr_t vp, bool preserve_fpu = true);
extern "C" BOOST_CONTEXT_DECL
fcontext_t * BOOST_CONTEXT_CALLDECL make_fcontext( void * sp, std::size_t size, void (* fn)( intptr_t) );

}}

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXT_FCONTEXT_H

