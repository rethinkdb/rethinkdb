#ifndef WINDOWS_HPP_
#define WINDOWS_HPP_

// ATN TODO

#ifdef _WIN32

#define _WINSOCKAPI_

#include <windows.h>
#include <wincrypt.h>

#include <winsock2.h>
#include <in6addr.h>

// defined by both Windows and RethinkDB
#undef DELETE
#undef OPTIONAL

// Windows uses different names for things
typedef SSIZE_T ssize_t;
const int PATH_MAX = MAX_PATH;

#endif /* _WIN32 */

#endif
