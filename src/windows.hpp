#ifndef WINDOWS_HPP_
#define WINDOWS_HPP_

/// @file windows.hpp
/// @brief Windows-specific headers and definitions
///
/// Centralizes Windows API includes and defines platform-specific constants.
/// Always include this file instead of <windows.h> directly to ensure consistent
/// macro definitions across the codebase.
///
/// @defgroup Windows Windows Platform Support
/// Windows API integration and compatibility
/// @{

#ifndef _WIN32
#error "Included windows.hpp, but this isn't windows"
#endif

/// @brief Restrict windows.h to essential APIs only
/// Prevents inclusion of rarely-used Win32 API parts that can define conflicting macros
#define WIN32_LEAN_AND_MEAN

/// @brief Disable secure CRT warnings on Visual C++
#define _SCL_SECURE_NO_WARNINGS

/// @brief Exclude GDI (Graphics Device Interface) APIs
#define NOGDI

#ifdef __MINGW32__
/// @brief Exclude USER APIs when building with MinGW
/// These break MinGW include files
#define NOUSER
#endif

/// @brief Prevent min/max macro conflicts
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Core Windows networking and socket headers
#include <winsock2.h>
#include <in6addr.h>

// Main Windows header
#include <windows.h>

// Additional Windows APIs
#include <wincrypt.h>      ///< Windows cryptography API
#include <mswsock.h>       ///< Microsoft Winsock extensions

/// @brief Undefine conflicting macros defined by Windows headers
/// RethinkDB uses these names for different purposes
#undef DELETE      ///< Windows defines DELETE, RethinkDB uses it differently
#undef OPTIONAL    ///< Windows defines OPTIONAL, RethinkDB uses it differently

#ifndef NOUSER
/// @brief Undefine additional User-related conflicts
#undef ERROR        ///< Windows ERROR constant conflicts with RethinkDB usage
#undef DIFFERENCE   ///< Windows DIFFERENCE conflicts with RethinkDB usage
#endif

/// @brief Windows uses different names for POSIX types
typedef SSIZE_T ssize_t;  ///< Map POSIX ssize_t to Windows SSIZE_T

/// @}

#endif // WINDOWS_HPP_

#ifdef _MSC_VER
// But mingw64 doesn't
const int PATH_MAX = MAX_PATH;
#endif

#endif
