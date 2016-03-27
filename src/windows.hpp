#ifndef WINDOWS_HPP_
#define WINDOWS_HPP_

// Always include this file instead of <windows.h>

#ifndef _WIN32
#error "Included windows.hpp, but this isn't windows"
#endif

// Restrict what gets imported by <windows.h>
#define WIN32_LEAN_AND_MEAN
#define _SCL_SECURE_NO_WARNINGS
#define NOGDI

#ifdef __MINGW32__
#define NOUSER  // this breaks mingw include files
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <in6addr.h>

#include <windows.h>
#include <wincrypt.h>
#include <mswsock.h>

// defined by both Windows and RethinkDB
#undef DELETE
#undef OPTIONAL

#ifndef NOUSER
#undef ERROR
#undef DIFFERENCE
#endif

// Windows uses different names for things
typedef SSIZE_T ssize_t;

#ifdef _MSC_VER
// But mingw64 doesn't
const int PATH_MAX = MAX_PATH;
#endif

#endif
