// Copyright 2012 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef V8_GLOBALS_H_
#define V8_GLOBALS_H_

// Define V8_INFINITY
#define V8_INFINITY INFINITY

// GCC specific stuff
#ifdef __GNUC__

#define __GNUC_VERSION_FOR_INFTY__ (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)

// Unfortunately, the INFINITY macro cannot be used with the '-pedantic'
// warning flag and certain versions of GCC due to a bug:
// http://gcc.gnu.org/bugzilla/show_bug.cgi?id=11931
// For now, we use the more involved template-based version from <limits>, but
// only when compiling with GCC versions affected by the bug (2.96.x - 4.0.x)
// __GNUC_PREREQ is not defined in GCC for Mac OS X, so we define our own macro
#if __GNUC_VERSION_FOR_INFTY__ >= 29600 && __GNUC_VERSION_FOR_INFTY__ < 40100
#include <limits>
#undef V8_INFINITY
#define V8_INFINITY std::numeric_limits<double>::infinity()
#endif
#undef __GNUC_VERSION_FOR_INFTY__

#endif  // __GNUC__

#ifdef _MSC_VER
#undef V8_INFINITY
#define V8_INFINITY HUGE_VAL
#endif


#include "../include/v8stdint.h"

namespace v8 {
namespace internal {

// Processor architecture detection.  For more info on what's defined, see:
//   http://msdn.microsoft.com/en-us/library/b0084kay.aspx
//   http://www.agner.org/optimize/calling_conventions.pdf
//   or with gcc, run: "echo | gcc -E -dM -"
#if defined(_M_X64) || defined(__x86_64__)
#define V8_HOST_ARCH_X64 1
#define V8_HOST_ARCH_64_BIT 1
#define V8_HOST_CAN_READ_UNALIGNED 1
#elif defined(_M_IX86) || defined(__i386__)
#define V8_HOST_ARCH_IA32 1
#define V8_HOST_ARCH_32_BIT 1
#define V8_HOST_CAN_READ_UNALIGNED 1
#elif defined(__ARMEL__)
#define V8_HOST_ARCH_ARM 1
#define V8_HOST_ARCH_32_BIT 1
// Some CPU-OS combinations allow unaligned access on ARM. We assume
// that unaligned accesses are not allowed unless the build system
// defines the CAN_USE_UNALIGNED_ACCESSES macro to be non-zero.
#if CAN_USE_UNALIGNED_ACCESSES
#define V8_HOST_CAN_READ_UNALIGNED 1
#endif
#elif defined(__MIPSEL__)
#define V8_HOST_ARCH_MIPS 1
#define V8_HOST_ARCH_32_BIT 1
#else
#error Host architecture was not detected as supported by v8
#endif

// Target architecture detection. This may be set externally. If not, detect
// in the same way as the host architecture, that is, target the native
// environment as presented by the compiler.
#if !defined(V8_TARGET_ARCH_X64) && !defined(V8_TARGET_ARCH_IA32) && \
    !defined(V8_TARGET_ARCH_ARM) && !defined(V8_TARGET_ARCH_MIPS)
#if defined(_M_X64) || defined(__x86_64__)
#define V8_TARGET_ARCH_X64 1
#elif defined(_M_IX86) || defined(__i386__)
#define V8_TARGET_ARCH_IA32 1
#elif defined(__ARMEL__)
#define V8_TARGET_ARCH_ARM 1
#elif defined(__MIPSEL__)
#define V8_TARGET_ARCH_MIPS 1
#else
#error Target architecture was not detected as supported by v8
#endif
#endif

// Check for supported combinations of host and target architectures.
#if defined(V8_TARGET_ARCH_IA32) && !defined(V8_HOST_ARCH_IA32)
#error Target architecture ia32 is only supported on ia32 host
#endif
#if defined(V8_TARGET_ARCH_X64) && !defined(V8_HOST_ARCH_X64)
#error Target architecture x64 is only supported on x64 host
#endif
#if (defined(V8_TARGET_ARCH_ARM) && \
    !(defined(V8_HOST_ARCH_IA32) || defined(V8_HOST_ARCH_ARM)))
#error Target architecture arm is only supported on arm and ia32 host
#endif
#if (defined(V8_TARGET_ARCH_MIPS) && \
    !(defined(V8_HOST_ARCH_IA32) || defined(V8_HOST_ARCH_MIPS)))
#error Target architecture mips is only supported on mips and ia32 host
#endif

// Determine whether we are running in a simulated environment.
// Setting USE_SIMULATOR explicitly from the build script will force
// the use of a simulated environment.
#if !defined(USE_SIMULATOR)
#if (defined(V8_TARGET_ARCH_ARM) && !defined(V8_HOST_ARCH_ARM))
#define USE_SIMULATOR 1
#endif
#if (defined(V8_TARGET_ARCH_MIPS) && !defined(V8_HOST_ARCH_MIPS))
#define USE_SIMULATOR 1
#endif
#endif

// Support for alternative bool type. This is only enabled if the code is
// compiled with USE_MYBOOL defined. This catches some nasty type bugs.
// For instance, 'bool b = "false";' results in b == true! This is a hidden
// source of bugs.
// However, redefining the bool type does have some negative impact on some
// platforms. It gives rise to compiler warnings (i.e. with
// MSVC) in the API header files when mixing code that uses the standard
// bool with code that uses the redefined version.
// This does not actually belong in the platform code, but needs to be
// defined here because the platform code uses bool, and platform.h is
// include very early in the main include file.

#ifdef USE_MYBOOL
typedef unsigned int __my_bool__;
#define bool __my_bool__  // use 'indirection' to avoid name clashes
#endif

typedef uint8_t byte;
typedef byte* Address;

// Define our own macros for writing 64-bit constants.  This is less fragile
// than defining __STDC_CONSTANT_MACROS before including <stdint.h>, and it
// works on compilers that don't have it (like MSVC).
#if V8_HOST_ARCH_64_BIT
#if defined(_MSC_VER)
#define V8_UINT64_C(x)  (x ## UI64)
#define V8_INT64_C(x)   (x ## I64)
#define V8_INTPTR_C(x)  (x ## I64)
#define V8_PTR_PREFIX "ll"
#elif defined(__MINGW64__)
#define V8_UINT64_C(x)  (x ## ULL)
#define V8_INT64_C(x)   (x ## LL)
#define V8_INTPTR_C(x)  (x ## LL)
#define V8_PTR_PREFIX "I64"
#else
#define V8_UINT64_C(x)  (x ## UL)
#define V8_INT64_C(x)   (x ## L)
#define V8_INTPTR_C(x)  (x ## L)
#define V8_PTR_PREFIX "l"
#endif
#else  // V8_HOST_ARCH_64_BIT
#define V8_INTPTR_C(x)  (x)
#define V8_PTR_PREFIX ""
#endif  // V8_HOST_ARCH_64_BIT

// The following macro works on both 32 and 64-bit platforms.
// Usage: instead of writing 0x1234567890123456
//      write V8_2PART_UINT64_C(0x12345678,90123456);
#define V8_2PART_UINT64_C(a, b) (((static_cast<uint64_t>(a) << 32) + 0x##b##u))

#define V8PRIxPTR V8_PTR_PREFIX "x"
#define V8PRIdPTR V8_PTR_PREFIX "d"
#define V8PRIuPTR V8_PTR_PREFIX "u"

// Fix for Mac OS X defining uintptr_t as "unsigned long":
#if defined(__APPLE__) && defined(__MACH__)
#undef V8PRIxPTR
#define V8PRIxPTR "lx"
#endif

#if (defined(__APPLE__) && defined(__MACH__)) || \
    defined(__FreeBSD__) || defined(__OpenBSD__)
#define USING_BSD_ABI
#endif

// -----------------------------------------------------------------------------
// Constants

const int KB = 1024;
const int MB = KB * KB;
const int GB = KB * KB * KB;
const int kMaxInt = 0x7FFFFFFF;
const int kMinInt = -kMaxInt - 1;

const uint32_t kMaxUInt32 = 0xFFFFFFFFu;

const int kCharSize     = sizeof(char);      // NOLINT
const int kShortSize    = sizeof(short);     // NOLINT
const int kIntSize      = sizeof(int);       // NOLINT
const int kDoubleSize   = sizeof(double);    // NOLINT
const int kIntptrSize   = sizeof(intptr_t);  // NOLINT
const int kPointerSize  = sizeof(void*);     // NOLINT

const int kDoubleSizeLog2 = 3;

// Size of the state of a the random number generator.
const int kRandomStateSize = 2 * kIntSize;

#if V8_HOST_ARCH_64_BIT
const int kPointerSizeLog2 = 3;
const intptr_t kIntptrSignBit = V8_INT64_C(0x8000000000000000);
const uintptr_t kUintptrAllBitsSet = V8_UINT64_C(0xFFFFFFFFFFFFFFFF);
#else
const int kPointerSizeLog2 = 2;
const intptr_t kIntptrSignBit = 0x80000000;
const uintptr_t kUintptrAllBitsSet = 0xFFFFFFFFu;
#endif

const int kBitsPerByte = 8;
const int kBitsPerByteLog2 = 3;
const int kBitsPerPointer = kPointerSize * kBitsPerByte;
const int kBitsPerInt = kIntSize * kBitsPerByte;

// IEEE 754 single precision floating point number bit layout.
const uint32_t kBinary32SignMask = 0x80000000u;
const uint32_t kBinary32ExponentMask = 0x7f800000u;
const uint32_t kBinary32MantissaMask = 0x007fffffu;
const int kBinary32ExponentBias = 127;
const int kBinary32MaxExponent  = 0xFE;
const int kBinary32MinExponent  = 0x01;
const int kBinary32MantissaBits = 23;
const int kBinary32ExponentShift = 23;

// Quiet NaNs have bits 51 to 62 set, possibly the sign bit, and no
// other bits set.
const uint64_t kQuietNaNMask = static_cast<uint64_t>(0xfff) << 51;

// ASCII/UTF-16 constants
// Code-point values in Unicode 4.0 are 21 bits wide.
// Code units in UTF-16 are 16 bits wide.
typedef uint16_t uc16;
typedef int32_t uc32;
const int kASCIISize    = kCharSize;
const int kUC16Size     = sizeof(uc16);      // NOLINT
const uc32 kMaxAsciiCharCode = 0x7f;
const uint32_t kMaxAsciiCharCodeU = 0x7fu;


// The expression OFFSET_OF(type, field) computes the byte-offset
// of the specified field relative to the containing type. This
// corresponds to 'offsetof' (in stddef.h), except that it doesn't
// use 0 or NULL, which causes a problem with the compiler warnings
// we have enabled (which is also why 'offsetof' doesn't seem to work).
// Here we simply use the non-zero value 4, which seems to work.
#define OFFSET_OF(type, field)                                          \
  (reinterpret_cast<intptr_t>(&(reinterpret_cast<type*>(4)->field)) - 4)


// The expression ARRAY_SIZE(a) is a compile-time constant of type
// size_t which represents the number of elements of the given
// array. You should only use ARRAY_SIZE on statically allocated
// arrays.
#define ARRAY_SIZE(a)                                   \
  ((sizeof(a) / sizeof(*(a))) /                         \
  static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))


// The USE(x) template is used to silence C++ compiler warnings
// issued for (yet) unused variables (typically parameters).
template <typename T>
inline void USE(T) { }


// FUNCTION_ADDR(f) gets the address of a C function f.
#define FUNCTION_ADDR(f)                                        \
  (reinterpret_cast<v8::internal::Address>(reinterpret_cast<intptr_t>(f)))


// FUNCTION_CAST<F>(addr) casts an address into a function
// of type F. Used to invoke generated code from within C.
template <typename F>
F FUNCTION_CAST(Address addr) {
  return reinterpret_cast<F>(reinterpret_cast<intptr_t>(addr));
}


// A macro to disallow the evil copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName)      \
  TypeName(const TypeName&);                    \
  void operator=(const TypeName&)


// A macro to disallow all the implicit constructors, namely the
// default constructor, copy constructor and operator= functions.
//
// This should be used in the private: declarations for a class
// that wants to prevent anyone from instantiating it. This is
// especially useful for classes containing only static methods.
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName();                                    \
  DISALLOW_COPY_AND_ASSIGN(TypeName)


// Define used for helping GCC to make better inlining. Don't bother for debug
// builds. On GCC 3.4.5 using __attribute__((always_inline)) causes compilation
// errors in debug build.
#if defined(__GNUC__) && !defined(DEBUG)
#if (__GNUC__ >= 4)
#define INLINE(header) inline header  __attribute__((always_inline))
#define NO_INLINE(header) header __attribute__((noinline))
#else
#define INLINE(header) inline __attribute__((always_inline)) header
#define NO_INLINE(header) __attribute__((noinline)) header
#endif
#elif defined(_MSC_VER) && !defined(DEBUG)
#define INLINE(header) __forceinline header
#define NO_INLINE(header) header
#else
#define INLINE(header) inline header
#define NO_INLINE(header) header
#endif


#if defined(__GNUC__) && __GNUC__ >= 4
#define MUST_USE_RESULT __attribute__ ((warn_unused_result))
#else
#define MUST_USE_RESULT
#endif


// Define DISABLE_ASAN macros.
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define DISABLE_ASAN __attribute__((no_address_safety_analysis))
#endif
#endif


#ifndef DISABLE_ASAN
#define DISABLE_ASAN
#endif


// -----------------------------------------------------------------------------
// Forward declarations for frequently used classes
// (sorted alphabetically)

class FreeStoreAllocationPolicy;
template <typename T, class P = FreeStoreAllocationPolicy> class List;

// -----------------------------------------------------------------------------
// Declarations for use in both the preparser and the rest of V8.

// The different language modes that V8 implements. ES5 defines two language
// modes: an unrestricted mode respectively a strict mode which are indicated by
// CLASSIC_MODE respectively STRICT_MODE in the enum. The harmony spec drafts
// for the next ES standard specify a new third mode which is called 'extended
// mode'. The extended mode is only available if the harmony flag is set. It is
// based on the 'strict mode' and adds new functionality to it. This means that
// most of the semantics of these two modes coincide.
//
// In the current draft the term 'base code' is used to refer to code that is
// neither in strict nor extended mode. However, the more distinguishing term
// 'classic mode' is used in V8 instead to avoid mix-ups.

enum LanguageMode {
  CLASSIC_MODE,
  STRICT_MODE,
  EXTENDED_MODE
};


// The Strict Mode (ECMA-262 5th edition, 4.2.2).
//
// This flag is used in the backend to represent the language mode. So far
// there is no semantic difference between the strict and the extended mode in
// the backend, so both modes are represented by the kStrictMode value.
enum StrictModeFlag {
  kNonStrictMode,
  kStrictMode
};


} }  // namespace v8::internal

#endif  // V8_GLOBALS_H_
