// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/type_id.hpp>
#include <boost/python/detail/decorated_type_id.hpp>
#include <utility>
#include <vector>
#include <algorithm>
#include <memory>
#include <cstdlib>
#include <cstring>

#if defined(__QNXNTO__)
# include <ostream>
#else                       /*  defined(__QNXNTO__) */

#if !defined(__GNUC__) || __GNUC__ >= 3 || __SGI_STL_PORT || __EDG_VERSION__
# include <ostream>
#else 
# include <ostream.h>
#endif 

#  ifdef BOOST_PYTHON_HAVE_GCC_CP_DEMANGLE
#   if defined(__GNUC__) &&  __GNUC__ >= 3

// http://lists.debian.org/debian-gcc/2003/09/msg00055.html notes
// that, in cxxabi.h of gcc-3.x for x < 4, this type is used before it
// is declared.
#    if __GNUC__ == 3 && __GNUC_MINOR__ < 4
class __class_type_info;
#    endif

#    include <cxxabi.h>
#   endif
#  endif 
#endif                      /*  defined(__QNXNTO__) */

namespace boost { namespace python {

#  ifdef BOOST_PYTHON_HAVE_GCC_CP_DEMANGLE

#   if defined(__QNXNTO__)
namespace cxxabi {
extern "C" char* __cxa_demangle(char const*, char*, std::size_t*, int*);
}
#   else                    /*  defined(__QNXNTO__) */

#    ifdef __GNUC__
#     if __GNUC__ < 3

namespace cxxabi = :: ;
extern "C" char* __cxa_demangle(char const*, char*, std::size_t*, int*);
#     else

namespace cxxabi = ::abi;       // GCC 3.1 and later

#      if __GNUC__ == 3 && __GNUC_MINOR__ == 0
namespace abi
{
  extern "C" char* __cxa_demangle(char const*, char*, std::size_t*, int*);
}
#      endif            /*  __GNUC__ == 3 && __GNUC_MINOR__ == 0    */
#     endif             /*  __GNUC__ < 3                            */
#    endif              /*  __GNUC__                                */
#   endif               /*  defined(__QNXNTO__)                     */

namespace
{
  struct compare_first_cstring
  {
      template <class T>
      bool operator()(T const& x, T const& y)
      {
          return std::strcmp(x.first,y.first) < 0;
      }
  };
  
  struct free_mem
  {
      free_mem(char*p)
          : p(p) {}
    
      ~free_mem()
      {
          std::free(p);
      }
      char* p;
  };
}

bool cxxabi_cxa_demangle_is_broken()
{
    static bool was_tested = false;
    static bool is_broken = false;
    if (!was_tested) {
        int status;
        free_mem keeper(cxxabi::__cxa_demangle("b", 0, 0, &status));
        was_tested = true;
        if (status == -2 || strcmp(keeper.p, "bool") != 0) {
          is_broken = true;
        }
    }
    return is_broken;
}

namespace detail
{
  BOOST_PYTHON_DECL char const* gcc_demangle(char const* mangled)
  {
      typedef std::vector<
          std::pair<char const*, char const*>
      > mangling_map;
      
      static mangling_map demangler;
      mangling_map::iterator p
          = std::lower_bound(
              demangler.begin(), demangler.end()
            , std::make_pair(mangled, (char const*)0)
            , compare_first_cstring());
      
      if (p == demangler.end() || strcmp(p->first, mangled))
      {
          int status;
          free_mem keeper(
              cxxabi::__cxa_demangle(mangled, 0, 0, &status)
              );
    
          assert(status != -3); // invalid argument error
    
          if (status == -1)
          {
              throw std::bad_alloc();
          }
          else
          {
              char const* demangled
                = status == -2
                  // Invalid mangled name.  Best we can do is to
                  // return it intact.
                  ? mangled
                  : keeper.p;

              // Ult Mundane, 2005 Aug 17
              // Contributed under the Boost Software License, Version 1.0.
              // (See accompanying file LICENSE_1_0.txt or copy at
              // http://www.boost.org/LICENSE_1_0.txt)
              // The __cxa_demangle function is supposed to translate
              // builtin types from their one-character mangled names,
              // but it doesn't in gcc 3.3.5 and gcc 3.4.x.
              if (cxxabi_cxa_demangle_is_broken()
                  && status == -2 && strlen(mangled) == 1)
              {
                  // list from
                  // http://www.codesourcery.com/cxx-abi/abi.html
                  switch (mangled[0])
                  {
                      case 'v': demangled = "void"; break;
                      case 'w': demangled = "wchar_t"; break;
                      case 'b': demangled = "bool"; break;
                      case 'c': demangled = "char"; break;
                      case 'a': demangled = "signed char"; break;
                      case 'h': demangled = "unsigned char"; break;
                      case 's': demangled = "short"; break;
                      case 't': demangled = "unsigned short"; break;
                      case 'i': demangled = "int"; break;
                      case 'j': demangled = "unsigned int"; break;
                      case 'l': demangled = "long"; break;
                      case 'm': demangled = "unsigned long"; break;
                      case 'x': demangled = "long long"; break;
                      case 'y': demangled = "unsigned long long"; break;
                      case 'n': demangled = "__int128"; break;
                      case 'o': demangled = "unsigned __int128"; break;
                      case 'f': demangled = "float"; break;
                      case 'd': demangled = "double"; break;
                      case 'e': demangled = "long double"; break;
                      case 'g': demangled = "__float128"; break;
                      case 'z': demangled = "..."; break;
                  }
              }

              p = demangler.insert(p, std::make_pair(mangled, demangled));
              keeper.p = 0;
          }
      }
      
      return p->second;
  }
}
#  endif

BOOST_PYTHON_DECL std::ostream& operator<<(std::ostream& os, type_info const& x)
{
    return os << x.name();
}

namespace detail
{
  BOOST_PYTHON_DECL std::ostream& operator<<(std::ostream& os, detail::decorated_type_info const& x)
  {
      os << x.m_base_type;
      if (x.m_decoration & decorated_type_info::const_)
          os << " const";
      if (x.m_decoration & decorated_type_info::volatile_)
          os << " volatile";
      if (x.m_decoration & decorated_type_info::reference)
          os << "&";
      return os;
  }
}
}} // namespace boost::python::converter
