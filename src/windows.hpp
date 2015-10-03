#ifndef WINDOWS_HPP_
#define WINDOWS_HPP_

// ATN TODO

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define _SCL_SECURE_NO_WARNINGS
#define NOGDI
// #define NOUSER // this breaks mingw include files

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <in6addr.h>

#include <windows.h>
#include <wincrypt.h>

// defined by both Windows and RethinkDB
#undef DELETE
#undef OPTIONAL
#undef ERROR
#undef DIFFERENCE

// Windows uses different names for things
typedef SSIZE_T ssize_t;

#ifdef _MSC_VER
// But mingw64 doesn't
const int PATH_MAX = MAX_PATH;
#endif

#endif /* _WIN32 */

#endif
