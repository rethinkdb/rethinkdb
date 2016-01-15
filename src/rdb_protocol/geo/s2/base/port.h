//
// Copyright (C) 1999 and onwards Google, Inc.
//
//
// These are weird things we need to do to get this compiling on
// random systems (and on SWIG).

#ifndef BASE_PORT_H_
#define BASE_PORT_H_

#include <limits.h>         // So we can set the bounds of our types
#include <string.h>         // for memcpy()
#include <stdlib.h>         // for free()

#if defined(__MACH__)
#include <unistd.h>         // for getpagesize() on mac
#endif

#include "rdb_protocol/geo/s2/base/integral_types.h"
#include "utils.hpp"

// Must happens before inttypes.h inclusion */
#if defined(__MACH__)
/* From MacOSX's inttypes.h:
 * "C++ implementations should define these macros only when
 *  __STDC_FORMAT_MACROS is defined before <inttypes.h> is included." */
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif  /* __STDC_FORMAT_MACROS */
#endif  /* __MACH__ */

/* Default for most OSes */
/* We use SIGPWR since that seems unlikely to be used for other reasons. */
#define GOOGLE_OBSCURE_SIGNAL  SIGPWR

#if defined __linux__ || defined OS_CYGWIN

// _BIG_ENDIAN
#include <endian.h>

// The uint mess:
// mysql.h sets _GNU_SOURCE which sets __USE_MISC in <features.h>
// sys/types.h typedefs uint if __USE_MISC
// mysql typedefs uint if HAVE_UINT not set
// The following typedef is carefully considered, and should not cause
//  any clashes
#if !defined(__USE_MISC)
#if !defined(HAVE_UINT)
#define HAVE_UINT 1
typedef unsigned int uint;
#endif
#if !defined(HAVE_USHORT)
#define HAVE_USHORT 1
typedef unsigned short ushort;
#endif
#if !defined(HAVE_ULONG)
#define HAVE_ULONG 1
typedef unsigned long ulong;
#endif
#endif

#if defined(__cplusplus)
#include <cstddef>              // For _GLIBCXX macros
#endif

#if !defined(HAVE_TLS) && defined(_GLIBCXX_HAVE_TLS) && defined(ARCH_K8)
#define HAVE_TLS 1
#endif

#elif defined OS_FREEBSD

// _BIG_ENDIAN
#include <machine/endian.h>

#elif defined OS_SOLARIS

// _BIG_ENDIAN
#include <sys/isa_defs.h>

// Solaris doesn't define sig_t (function taking an int, returning void)
typedef void (*sig_t)(int);

// Solaris only defines strtoll, not strtoq
#define strtoq  strtoll
#define strtouq strtoull

// It doesn't define the posix-standard(?) u_int_16
#include <sys/int_types.h>
typedef uint16_t u_int16_t;

#elif defined __MACH__

// BIG_ENDIAN
#include <machine/endian.h>
/* Let's try and follow the Linux convention */
#define __BYTE_ORDER  BYTE_ORDER
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#define __BIG_ENDIAN BIG_ENDIAN

#endif

#ifdef _WIN32
#include <stdlib.h>
#define __BYTE_ORDER __LITTLE_ENDIAN
#define bswap_16(x) _byteswap_ushort(x)
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)

#elif defined(__MACH__)
// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define bswap_16(x) OSSwapInt16(x)
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)

#else
#include <byteswap.h>
#endif


// define the macros IS_LITTLE_ENDIAN or IS_BIG_ENDIAN
// using the above endian defintions from endian.h if
// endian.h was included
#ifdef __BYTE_ORDER
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define IS_LITTLE_ENDIAN
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
#define IS_BIG_ENDIAN
#endif

#else

#if defined(__LITTLE_ENDIAN__)
#define IS_LITTLE_ENDIAN
#elif defined(__BIG_ENDIAN__)
#define IS_BIG_ENDIAN
#endif

// there is also PDP endian ...

#endif  // __BYTE_ORDER

// Define the OS's path separator
#ifdef __cplusplus  // C won't merge duplicate const variables at link time
// Some headers provide a macro for this (GCC's system.h), remove it so that we
// can use our own.
#undef PATH_SEPARATOR
#if defined(OS_WINDOWS) && OS_WINDOWS
const char PATH_SEPARATOR = '\\';
#else
const char PATH_SEPARATOR = '/';
#endif
#endif

// Windows has O_BINARY as a flag to open() (like "b" for fopen).
// Linux doesn't need make this distinction.
#if defined __linux__ && !defined O_BINARY
#define O_BINARY 0
#endif

// va_copy portability definitions
#ifdef COMPILER_MSVC

#if _MSC_VER < 1400
// MSVC doesn't have va_copy yet.
// This is believed to work for 32-bit msvc.  This may not work at all for
// other platforms.
// If va_list uses the single-element-array trick, you will probably get
// a compiler error here.
//
#include <stdarg.h>
inline void va_copy(va_list& a, va_list& b) {
  a = b;
}
#endif

// Nor does it have uid_t
typedef int uid_t;

#endif

// Mac OS X / Darwin features

#if defined(__MACH__)

// For mmap, Linux defines both MAP_ANONYMOUS and MAP_ANON and says MAP_ANON is
// deprecated. In Darwin, MAP_ANON is all there is.
#if !defined MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

// Linux has this in <sys/cdefs.h>
#define __ptr_t void *

// Linux has this in <linux/errno.h>
#define EXFULL      ENOMEM  // not really that great a translation...

// Mach-O supports sections (albeit with small names), but doesn't have
// vars at the beginning and end.  Instead you should call the function
// getsectdata("__DATA", name, &size).
#define HAVE_ATTRIBUTE_SECTION 1

// Any function with ATTRIBUTE_SECTION must not be inlined, or it will
// be placed into whatever section its caller is placed into.
#define ATTRIBUTE_SECTION(name) \
  __attribute__ ((section ("__DATA, " #name))) __attribute__ ((noinline))

#define ENUM_DYLD_BOOL  // so that we don't pollute the global namespace
extern "C" {
  #include <mach-o/getsect.h>
  #include <mach-o/dyld.h>
}
class AssignAttributeStartEnd {
 public:
  AssignAttributeStartEnd(const char* name, char** pstart, char** pend) {
    // Find out what dynamic library name is defined in
    for (int i = _dyld_image_count() - 1; i >= 0; --i) {
      const mach_header* hdr = _dyld_get_image_header(i);
      uint32_t len;
      *pstart = getsectdatafromheader(hdr, "__DATA", name, &len);
      if (*pstart) {   // NULL if not defined in this dynamic library
        *pstart += _dyld_get_image_vmaddr_slide(i);   // correct for reloc
        *pend = *pstart + len;
        return;
      }
    }
    // If we get here, not defined in a dll at all.  See if defined statically.
    unsigned long len;    // don't ask me why this type isn't uint32_t too...
    *pstart = getsectdata("__DATA", name, &len);
    *pend = *pstart + len;
  }
};

// 1) DEFINE_ATTRIBUTE_SECTION_VARS: must be called once per unique
//    name.  You want to make sure this is executed before any
//    DECLARE_ATTRIBUTE_SECTION_VARS; the easiest way is to put them
//    in the same .cc file.  Put this call at the global level.
// 2) INIT_ATTRIBUTE_SECTION_VARS: you can scatter calls to this in
//    multiple places to help ensure execution before any
//    DECLARE_ATTRIBUTE_SECTION_VARS.  You must have at least one
//    DEFINE, but you can have many INITs.  Put each in its own scope.
// 3) DECLARE_ATTRIBUTE_SECTION_VARS: must be called before using
//    ATTRIBUTE_SECTION_START or ATTRIBUTE_SECTION_STOP on a name.
//    Put this call at the global level.
#define DECLARE_ATTRIBUTE_SECTION_VARS(name) \
  extern char* __start_##name; \
  extern char* __stop_##name;

#define INIT_ATTRIBUTE_SECTION_VARS(name)               \
  DECLARE_ATTRIBUTE_SECTION_VARS(name);                 \
  static const AssignAttributeStartEnd __assign_##name( \
    #name, &__start_##name, &__stop_##name)

#define DEFINE_ATTRIBUTE_SECTION_VARS(name)             \
  char* __start_##name, *__stop_##name;                 \
  INIT_ATTRIBUTE_SECTION_VARS(name)

// Darwin doesn't have strnlen. No comment.
inline size_t strnlen(const char *s, size_t maxlen) {
  const char* end = (const char *)memchr(s, '\0', maxlen);
  if (end)
    return end - s;
  return maxlen;
}

// Doesn't exist on OSX; used in google.cc for send() to mean "no flags".
#define MSG_NOSIGNAL 0

// No SIGPWR on MacOSX.  SIGINFO seems suitably obscure.
#undef GOOGLE_OBSCURE_SIGNAL
#define GOOGLE_OBSCURE_SIGNAL  SIGINFO

#elif defined(OS_CYGWIN)  // Cygwin-specific behavior.

#if defined(__CYGWIN32__)
#define __WORDSIZE 32
#else
// It's probably possible to support 64-bit, but the #defines will need checked.
#error "Cygwin is currently only 32-bit."
#endif

// No signalling on Windows.
#undef GOOGLE_OBSCURE_SIGNAL
#define GOOGLE_OBSCURE_SIGNAL 0

struct stack_t {
  void* ss_sp;
  int ss_flags;
  size_t ss_size;
};
inline int sigaltstack(stack_t* ss, stack_t* oss) { return 0; }

#define PTHREAD_STACK_MIN 0  // Not provided by cygwin

// Scans memory for a character.
// memrchr is used in a few places, but it's linux-specific.
inline void* memrchr(const void* bytes, int find_char, size_t len) {
  const unsigned char* cursor =
      reinterpret_cast<const unsigned char*>(bytes) + len - 1;
  unsigned char actual_char = find_char;
  for (; cursor >= bytes; --cursor) {
    if (*cursor == actual_char) {
      return const_cast<void*>(reinterpret_cast<const void*>(cursor));
    }
  }
  return NULL;
}

#endif

// Klocwork static analysis tool's C/C++ complier kwcc
#if defined(__KLOCWORK__)
#define STATIC_ANALYSIS
#endif // __KLOCWORK__

// GCC-specific features

#if (defined(__GNUC__) || defined(__MACH__)) && !defined(SWIG)

//
// Prevent the compiler from padding a structure to natural alignment
//
#define PACKED __attribute__ ((packed))

// Cache line alignment
#if defined(__i386__) || defined(__x86_64__)
#define CACHELINE_SIZE 64
#define CACHELINE_ALIGNED __attribute__((aligned(CACHELINE_SIZE)))
#elif defined(__ARM_ARCH_5T__)
#define CACHELINE_SIZE 32
#define CACHELINE_ALIGNED
#else
#define CACHELINE_ALIGNED
#endif

//
// Prevent the compiler from complaining about or optimizing away variables
// that appear unused
// (careful, others e.g. third_party/libxml/xmlversion.h also define this)
#undef ATTRIBUTE_UNUSED
#define ATTRIBUTE_UNUSED __attribute__ ((unused))

//
// For functions we want to force inline or not inline.
// Introduced in gcc 3.1.
#define ATTRIBUTE_ALWAYS_INLINE  __attribute__ ((always_inline))
#define HAVE_ATTRIBUTE_ALWAYS_INLINE 1
#define ATTRIBUTE_NOINLINE __attribute__ ((noinline))
#define HAVE_ATTRIBUTE_NOINLINE 1

// For weak functions
#undef ATTRIBUTE_WEAK
#define ATTRIBUTE_WEAK __attribute__ ((weak))
#define HAVE_ATTRIBUTE_WEAK 1

// Tell the compiler to use "initial-exec" mode for a thread-local variable.
// See http://people.redhat.com/drepper/tls.pdf for the gory details.
#define ATTRIBUTE_INITIAL_EXEC __attribute__ ((tls_model ("initial-exec")))

//
// Tell the compiler that a given function never returns
//
#define ATTRIBUTE_NORETURN __attribute__((noreturn))

// For deprecated functions, variables, and types.
// gcc 3.1.1 and later provide this attribute.
// gcc 3.1.1 and later provide -Wdeprecated-declarations, on by default,
//   and then -Werror converts such warning to an error
// gcc 4.2.1 and later provide -Wno-error=deprecated-declarations,
//   so that use of a deprecated entity is a warning but not an error
//
// gcc 4.2.1 and gcc 4.2.2 ignore ATTRIBUTE_DEPRECATED on virtual functions.
// this is fixed in gcc 4.3.1 (crosstool v12).  -- mec, 2008-10-21
//
// 2010-05-19(mec): Failed.
// Too many people started deprecations and then stopped working on them.
// The deprecation messages just became build noise.
// The two-part deletion plan:
//   change definition of ATTRIBUTE_DEPRECATED to an empty macro
//   then global change: ATTRIBUTE_DEPRECATED -> /* deprecated */
// We may introduce a new facility like this in the future,
// probably with a different name.  See message from iant to c-style:
#define ATTRIBUTE_DEPRECATED

#ifndef HAVE_ATTRIBUTE_SECTION  // may have been pre-set to 0, e.g. for Darwin
#define HAVE_ATTRIBUTE_SECTION 1
#endif

#if HAVE_ATTRIBUTE_SECTION  // define section support for the case of GCC

//
// Tell the compiler/linker to put a given function into a section and define
// "__start_ ## name" and "__stop_ ## name" symbols to bracket the section.
// Sections can not span more than none compilation unit.
// This functionality is supported by GNU linker.
// Any function with ATTRIBUTE_SECTION must not be inlined, or it will
// be placed into whatever section its caller is placed into.
//
#ifndef ATTRIBUTE_SECTION
#define ATTRIBUTE_SECTION(name) \
  __attribute__ ((section (#name))) __attribute__ ((noinline))
#endif

//
// Weak section declaration to be used as a global declaration
// for ATTRIBUTE_SECTION_START|STOP(name) to compile and link
// even without functions with ATTRIBUTE_SECTION(name).
// DEFINE_ATTRIBUTE_SECTION should be in the exactly one file; it's
// a no-op on ELF but not on Mach-O.
//
#ifndef DECLARE_ATTRIBUTE_SECTION_VARS
#define DECLARE_ATTRIBUTE_SECTION_VARS(name) \
  extern char __start_##name[] ATTRIBUTE_WEAK; \
  extern char __stop_##name[] ATTRIBUTE_WEAK
#endif
#ifndef DEFINE_ATTRIBUTE_SECTION_VARS
#define INIT_ATTRIBUTE_SECTION_VARS(name)
#define DEFINE_ATTRIBUTE_SECTION_VARS(name)
#endif

//
// Return void* pointers to start/end of a section of code with
// functions having ATTRIBUTE_SECTION(name).
// Returns 0 if no such functions exits.
// One must DECLARE_ATTRIBUTE_SECTION_VARS(name) for this to compile and link.
//
#define ATTRIBUTE_SECTION_START(name) (reinterpret_cast<void*>(__start_##name))
#define ATTRIBUTE_SECTION_STOP(name) (reinterpret_cast<void*>(__stop_##name))

#endif  // HAVE_ATTRIBUTE_SECTION

//
// The legacy prod71 libc does not provide the stack alignment required for use
// of SSE intrinsics.  In order to properly use the intrinsics you need to use
// a trampoline function which aligns the stack prior to calling your code,
// or as of crosstool v10 with gcc 4.2.0 there is an attribute which asks
// gcc to do this for you.
//
// It has also been discovered that crosstool up to and including v10 does not
// provide proper alignment for pthread_once() functions in x86-64 code either.
// Unfortunately gcc does not provide force_align_arg_pointer as an option in
// x86-64 code, so this requires us to always have a trampoline.
//
// For an example of using this see util/hash/adler32*

#if defined(__i386__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
#define ATTRIBUTE_STACK_ALIGN_FOR_OLD_LIBC __attribute__((force_align_arg_pointer))
#define REQUIRE_STACK_ALIGN_TRAMPOLINE (0)
#elif defined(__i386__) || defined(__x86_64__)
#define REQUIRE_STACK_ALIGN_TRAMPOLINE (1)
#define ATTRIBUTE_STACK_ALIGN_FOR_OLD_LIBC
#else
#define REQUIRE_STACK_ALIGN_TRAMPOLINE (0)
#define ATTRIBUTE_STACK_ALIGN_FOR_OLD_LIBC
#endif


//
// Tell the compiler to warn about unused return values for functions declared
// with this macro.  The macro should be used on function declarations
// following the argument list:
//
//   Sprocket* AllocateSprocket() MUST_USE_RESULT;
//
#undef MUST_USE_RESULT
#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)) \
  && !defined(COMPILER_ICC)
#define MUST_USE_RESULT __attribute__ ((warn_unused_result))
#else
#define MUST_USE_RESULT
#endif

#define FTELLO ftello
#define FSEEKO fseeko

#else   // not GCC

#define SCANF_ATTRIBUTE(string_index, first_to_check)
#define PACKED
#define CACHELINE_ALIGNED
#define ATTRIBUTE_UNUSED
#define ATTRIBUTE_ALWAYS_INLINE
#define ATTRIBUTE_NOINLINE
#define ATTRIBUTE_WEAK
#define HAVE_ATTRIBUTE_WEAK 0
#define ATTRIBUTE_INITIAL_EXEC
#define ATTRIBUTE_NORETURN
#define ATTRIBUTE_DEPRECATED
#define HAVE_ATTRIBUTE_SECTION 0
#define ATTRIBUTE_STACK_ALIGN_FOR_OLD_LIBC
#define REQUIRE_STACK_ALIGN_TRAMPOLINE (0)
#define MUST_USE_RESULT

// These should be redefined appropriately if better alternatives to
// ftell/fseek exist in the compiler
#define FTELLO ftell
#define FSEEKO fseek

#endif  // GCC

// Disable printf format checking in general, because it's too strict.
#define PRINTF_ATTRIBUTE(string_index, first_to_check)

#if !HAVE_ATTRIBUTE_SECTION  // provide dummy definitions

#define ATTRIBUTE_SECTION(name)
#define INIT_ATTRIBUTE_SECTION_VARS(name)
#define DEFINE_ATTRIBUTE_SECTION_VARS(name)
#define DECLARE_ATTRIBUTE_SECTION_VARS(name)
#define ATTRIBUTE_SECTION_START(name) (reinterpret_cast<void*>(0))
#define ATTRIBUTE_SECTION_STOP(name) (reinterpret_cast<void*>(0))

#endif  // !HAVE_ATTRIBUTE_SECTION

#ifdef STL_MSVC  // not always the same as COMPILER_MSVC
#include "rdb_protocol/geo/s2/base/port_hash.h"  // NOPROCESS
#else
struct PortableHashBase { };
#endif

// The SWIGged version of an abstract class must be concrete if any methods
// return objects of the abstract type.
//
// This location is deprecated, the new preferred location is in base/macros.h.
#ifndef SWIG
#define ABSTRACT = 0
#endif


#if defined(OS_WINDOWS) || defined(__MACH__)
// gethostbyname() *is* thread-safe for Windows native threads. It is also
// safe on Mac OS X, where it uses thread-local storage, even though the
// manpages claim otherwise. For details, see
// http://lists.apple.com/archives/Darwin-dev/2006/May/msg00008.html
#else
// gethostbyname() is not thread-safe.  So disallow its use.  People
// should either use the HostLookup::Lookup*() methods, or gethostbyname_r()
#define gethostbyname gethostbyname_is_not_thread_safe_DO_NOT_USE
#endif

// create macros in which the programmer should enclose all specializations
// for unordered_maps and unordered_sets. This is necessary since these classes are not
// STL standardized. Depending on the STL implementation they are in different
// namespaces. Right now the right namespace is passed by the Makefile
// Examples: gcc3: -DHASH_NAMESPACE=__gnu_cxx
//           icc:  -DHASH_NAMESPACE=std
//           gcc2: empty

// C++11
#define HASH_NAMESPACE std

#ifndef HASH_NAMESPACE
#  define HASH_NAMESPACE_DECLARATION_START
#  define HASH_NAMESPACE_DECLARATION_END
#else
#  define HASH_NAMESPACE_DECLARATION_START  namespace HASH_NAMESPACE {
#  define HASH_NAMESPACE_DECLARATION_END    }
#endif

// Our STL-like classes use __STD.
#if defined(__GNUC__) || defined(__MACH__) || defined(COMPILER_MSVC)
#define __STD std
#endif

#if defined __GNUC__
#define STREAM_SET(s, bit) (s).setstate(ios_base::bit)
#define STREAM_SETF(s, flag) (s).setf(ios_base::flag)
#else
#define STREAM_SET(s, bit) (s).set(ios::bit)
#define STREAM_SETF(s, flag) (s).setf(ios::flag)
#endif

// Portable handling of unaligned loads and stores

#if defined(ARCH_PIII) || defined(ARCH_ATHLON) || defined(ARCH_K8) || defined(_ARCH_PPC)

// x86 and x86-64 can perform unaligned loads/stores directly;
// modern PowerPC hardware can also do unaligned integer loads and stores;
// but note: the FPU still sends unaligned loads and stores to a trap handler!

#define UNALIGNED_LOAD16(_p) (*reinterpret_cast<const uint16 *>(_p))
#define UNALIGNED_LOAD32(_p) (*reinterpret_cast<const uint32 *>(_p))
#define UNALIGNED_LOAD64(_p) (*reinterpret_cast<const uint64 *>(_p))

#define UNALIGNED_STORE16(_p, _val) (*reinterpret_cast<uint16 *>(_p) = (_val))
#define UNALIGNED_STORE32(_p, _val) (*reinterpret_cast<uint32 *>(_p) = (_val))
#define UNALIGNED_STORE64(_p, _val) (*reinterpret_cast<uint64 *>(_p) = (_val))

#else

#define NEED_ALIGNED_LOADS

// These functions are provided for architectures that don't support
// unaligned loads and stores.

namespace geo {

inline uint16 UNALIGNED_LOAD16(const void *p) {
  uint16 t;
  memcpy(&t, p, sizeof t);
  return t;
}

inline uint32 UNALIGNED_LOAD32(const void *p) {
  uint32 t;
  memcpy(&t, p, sizeof t);
  return t;
}

inline uint64 UNALIGNED_LOAD64(const void *p) {
  uint64 t;
  memcpy(&t, p, sizeof t);
  return t;
}

inline void UNALIGNED_STORE16(void *p, uint16 v) {
  memcpy(p, &v, sizeof v);
}

inline void UNALIGNED_STORE32(void *p, uint32 v) {
  memcpy(p, &v, sizeof v);
}

inline void UNALIGNED_STORE64(void *p, uint64 v) {
  memcpy(p, &v, sizeof v);
}

}  // namespace geo
#endif

#ifdef _LP64
#define UNALIGNED_LOADW(_p) UNALIGNED_LOAD64(_p)
#define UNALIGNED_STOREW(_p, _val) UNALIGNED_STORE64(_p, _val)
#else
#define UNALIGNED_LOADW(_p) UNALIGNED_LOAD32(_p)
#define UNALIGNED_STOREW(_p, _val) UNALIGNED_STORE32(_p, _val)
#endif

// printf macros for size_t, in the style of inttypes.h
#ifdef _LP64
#define __PRIS_PREFIX "z"
#else
#define __PRIS_PREFIX
#endif

// Use these macros after a % in a printf format string
// to get correct 32/64 bit behavior, like this:
// size_t size = records.size();
// printf("%"PRIuS"\n", size);

#define PRIdS __PRIS_PREFIX "d"
#define PRIxS __PRIS_PREFIX "x"
#define PRIuS __PRIS_PREFIX "u"
#define PRIXS __PRIS_PREFIX "X"
#define PRIoS __PRIS_PREFIX "o"

#define GPRIuPTHREAD "lu"
#define GPRIxPTHREAD "lx"
#ifdef OS_CYGWIN
#define PRINTABLE_PTHREAD(pthreadt) reinterpret_cast<uintptr_t>(pthreadt)
#else
#define PRINTABLE_PTHREAD(pthreadt) pthreadt
#endif

#define SIZEOF_MEMBER(t, f)   sizeof(((t*) 4096)->f)

#define OFFSETOF_MEMBER(t, f)         \
  (reinterpret_cast<char*>(           \
     &reinterpret_cast<t*>(16)->f) -  \
   reinterpret_cast<char*>(16))

#ifdef PTHREADS_REDHAT_WIN32
#include <iosfwd>

#include <pthread.h>
// pthread_t is not a simple integer or pointer on Win32
std::ostream& operator << (std::ostream& out, const pthread_t& thread_id);
#endif

#endif  // BASE_PORT_H_
