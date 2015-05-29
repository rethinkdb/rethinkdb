#ifndef WINDOWS_HPP_
#define WINDOWS_HPP_

// ATN TODO

#ifdef _WIN32

#define _WINSOCKAPI_

#include <windows.h>
#include <wincrypt.h>
// #include <winsock.h> // ATN TODO
#include <winsock2.h>
#include <in6addr.h>

// defined by both Windows and RethinkDB
#undef DELETE

#endif /* _WIN32 */

#endif