#ifndef UTYPEINFO_H
#define UTYPEINFO_H

#if defined(_MSC_VER) && _HAS_EXCEPTIONS == 0
// Visual C++ needs the following two lines when RTTI is on with
// exception handling disabled.
#include <exception>
using std::exception;
#endif
#include <typeinfo> // for typeid to work.

#endif
