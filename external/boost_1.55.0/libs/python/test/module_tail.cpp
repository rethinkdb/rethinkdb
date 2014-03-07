// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if defined(_WIN32)
# ifdef __MWERKS__
#  pragma ANSI_strict off
# endif
# include <windows.h>
# ifdef __MWERKS__
#  pragma ANSI_strict reset
# endif

# ifdef _MSC_VER
#  include <eh.h> // for _set_se_translator()
#  pragma warning(push)
#  pragma warning(disable:4297)
#  pragma warning(disable:4535)
extern "C" void straight_to_debugger(unsigned int, EXCEPTION_POINTERS*)
{
    throw;
}
extern "C" void (*old_translator)(unsigned, EXCEPTION_POINTERS*)
         = _set_se_translator(straight_to_debugger);
#  pragma warning(pop)
# endif

#endif // _WIN32

#include <exception>
#include <boost/python/extract.hpp>
#include <boost/python/str.hpp>
struct test_failure : std::exception
{
    test_failure(char const* expr, char const* /*function*/, char const* file, unsigned line)
      : msg(file + boost::python::str(":%s:") % line + ": Boost.Python assertion failure: " + expr)
    {}

    ~test_failure() throw() {}
    
    char const* what() const throw()
    {
        return boost::python::extract<char const*>(msg)();
    }

    boost::python::str msg;
};

namespace boost
{

void assertion_failed(char const * expr, char const * function, char const * file, long line)
{
    throw ::test_failure(expr,function, file, line);
}

} // namespace boost
