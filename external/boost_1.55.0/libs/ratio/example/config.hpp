//  boost/chrono/config.hpp  -------------------------------------------------//

//  Copyright Beman Dawes 2003, 2006, 2008
//  Copyright 2009 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/chrono for documentation.

#ifndef BOOST_EX_CHRONO_CONFIG_HPP
#define BOOST_EX_CHRONO_CONFIG_HPP

#include <boost/config.hpp>

//  BOOST_EX_CHRONO_POSIX_API, BOOST_EX_CHRONO_MAC_API, or BOOST_EX_CHRONO_WINDOWS_API
//  can be defined by the user to specify which API should be used

#if defined(BOOST_EX_CHRONO_WINDOWS_API)
# warning Boost.Chrono will use the Windows API
#elif defined(BOOST_EX_CHRONO_MAC_API)
# warning Boost.Chrono will use the Mac API
#elif defined(BOOST_EX_CHRONO_POSIX_API)
# warning Boost.Chrono will use the POSIX API
#endif

# if defined( BOOST_EX_CHRONO_WINDOWS_API ) && defined( BOOST_EX_CHRONO_POSIX_API )
#   error both BOOST_EX_CHRONO_WINDOWS_API and BOOST_EX_CHRONO_POSIX_API are defined
# elif defined( BOOST_EX_CHRONO_WINDOWS_API ) && defined( BOOST_EX_CHRONO_MAC_API )
#   error both BOOST_EX_CHRONO_WINDOWS_API and BOOST_EX_CHRONO_MAC_API are defined
# elif defined( BOOST_EX_CHRONO_MAC_API ) && defined( BOOST_EX_CHRONO_POSIX_API )
#   error both BOOST_EX_CHRONO_MAC_API and BOOST_EX_CHRONO_POSIX_API are defined
# elif !defined( BOOST_EX_CHRONO_WINDOWS_API ) && !defined( BOOST_EX_CHRONO_MAC_API ) && !defined( BOOST_EX_CHRONO_POSIX_API )
#   if (defined(_WIN32) || defined(__WIN32__) || defined(WIN32))
#     define BOOST_EX_CHRONO_WINDOWS_API
#     define BOOST_EX_CHRONO_HAS_CLOCK_MONOTONIC
#     define BOOST_EX_CHRONO_HAS_THREAD_CLOCK
#     define BOOST_EX_CHRONO_THREAD_CLOCK_IS_MONOTONIC true
#   elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#     define BOOST_EX_CHRONO_MAC_API
#     define BOOST_EX_CHRONO_HAS_CLOCK_MONOTONIC
#     define BOOST_EX_CHRONO_THREAD_CLOCK_IS_MONOTONIC true
#   else
#     define BOOST_EX_CHRONO_POSIX_API
#   endif
# endif

# if defined( BOOST_EX_CHRONO_POSIX_API )
#   include <time.h>  //to check for CLOCK_REALTIME and CLOCK_MONOTONIC and _POSIX_THREAD_CPUTIME
#   if defined(CLOCK_REALTIME)
#     if defined(CLOCK_MONOTONIC)
#        define BOOST_EX_CHRONO_HAS_CLOCK_MONOTONIC
#     endif
#   else
#     error <time.h> does not supply CLOCK_REALTIME
#   endif
#   if defined(_POSIX_THREAD_CPUTIME)
#     define BOOST_EX_CHRONO_HAS_THREAD_CLOCK
#     define BOOST_EX_CHRONO_THREAD_CLOCK_IS_MONOTONIC true
#   endif
# endif



//  enable dynamic linking on Windows  ---------------------------------------//

//#  if (defined(BOOST_ALL_DYN_LINK) || defined(BOOST_EX_CHRONO_DYN_LINK)) && defined(__BORLANDC__) && defined(__WIN32__)
//#    error Dynamic linking Boost.System does not work for Borland; use static linking instead
//#  endif

#ifdef BOOST_HAS_DECLSPEC // defined by boost.config
// we need to import/export our code only if the user has specifically
// asked for it by defining either BOOST_ALL_DYN_LINK if they want all boost
// libraries to be dynamically linked, or BOOST_EX_CHRONO_DYN_LINK
// if they want just this one to be dynamically liked:
#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_EX_CHRONO_DYN_LINK)
// export if this is our own source, otherwise import:
#ifdef BOOST_EX_CHRONO_SOURCE
# define BOOST_EX_CHRONO_DECL __declspec(dllexport)
#else
# define BOOST_EX_CHRONO_DECL __declspec(dllimport)
#endif  // BOOST_EX_CHRONO_SOURCE
#endif  // DYN_LINK
#endif  // BOOST_HAS_DECLSPEC
//
// if BOOST_EX_CHRONO_DECL isn't defined yet define it now:
#ifndef BOOST_EX_CHRONO_DECL
#define BOOST_EX_CHRONO_DECL
#endif

//  define constexpr related macros  ------------------------------//

//~ #include <boost/config.hpp>
#if defined(BOOST_NO_CXX11_CONSTEXPR)
#define BOOST_EX_CHRONO_CONSTEXPR
#define BOOST_EX_CHRONO_CONST_REF const&
#else
#define BOOST_EX_CHRONO_CONSTEXPR constexpr
#define BOOST_EX_CHRONO_CONST_REF
#endif

//  enable automatic library variant selection  ------------------------------//

#if !defined(BOOST_EX_CHRONO_SOURCE) && !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_EX_CHRONO_NO_LIB)
//
// Set the name of our library; this will get undef'ed by auto_link.hpp
// once it's done with it:
//
#define BOOST_LIB_NAME boost_chrono
//
// If we're importing code from a dll, then tell auto_link.hpp about it:
//
#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_EX_CHRONO_DYN_LINK)
#  define BOOST_DYN_LINK
#endif
//
// And include the header that does the work:
//
#include <boost/config/auto_link.hpp>
#endif  // auto-linking disabled

#endif // BOOST_EX_CHRONO_CONFIG_HPP

