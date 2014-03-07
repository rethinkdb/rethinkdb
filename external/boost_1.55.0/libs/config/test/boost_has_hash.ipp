//  (C) Copyright John Maddock 2001.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_HASH
//  TITLE:         <hashset> and <hashmap>
//  DESCRIPTION:   The C++ implementation provides the (SGI) hash_set
//                 or hash_map classes.

#if defined(__GLIBCXX__) || (defined(__GLIBCPP__) && __GLIBCPP__>=20020514) // GCC >= 3.1.0
#  ifdef BOOST_NO_CXX11_STD_UNORDERED
#    define BOOST_STD_EXTENSION_NAMESPACE __gnu_cxx
#    define _BACKWARD_BACKWARD_WARNING_H 1 /* turn off warnings from the headers below */
#    include <ext/hash_set>
#    include <ext/hash_map>
#  else
     // If we have BOOST_NO_CXX11_STD_UNORDERED *not* defined, then we must
     // not include the <ext/*> headers as they clash with the C++0x
     // headers.  ie in any given translation unit we can include one
     // or the other, but not both.
#    define DISABLE_BOOST_HAS_HASH_TEST
#  endif
#else
#include <hash_set>
#include <hash_map>
#endif

#ifndef BOOST_STD_EXTENSION_NAMESPACE
#define BOOST_STD_EXTENSION_NAMESPACE std
#endif

namespace boost_has_hash{

#ifndef DISABLE_BOOST_HAS_HASH_TEST

template <class Key, class Eq, class Hash, class Alloc>
void foo(const BOOST_STD_EXTENSION_NAMESPACE::hash_set<Key,Eq,Hash,Alloc>& )
{
}

template <class Key, class T, class Eq, class Hash, class Alloc>
void foo(const BOOST_STD_EXTENSION_NAMESPACE::hash_map<Key,T,Eq,Hash,Alloc>& )
{
}

#endif

int test()
{
#ifndef DISABLE_BOOST_HAS_HASH_TEST
   BOOST_STD_EXTENSION_NAMESPACE::hash_set<int> hs;
   foo(hs);
   BOOST_STD_EXTENSION_NAMESPACE::hash_map<int, long> hm;
   foo(hm);
#endif
   return 0;
}

}






