/*
 *
 * Copyright (c) 2003
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

//
// This program extends config_info to print out regex library
// configuration information.  We do this by redfining the main
// provided by config_info, our real main will call it later:
//
#ifndef OLD_MAIN
#  define OLD_MAIN info_main
#endif

#define main OLD_MAIN
#include <libs/config/test/config_info.cpp>
#undef main
#ifndef NEW_MAIN
#  define NEW_MAIN main
#endif
#include <boost/regex.hpp>

int NEW_MAIN()
{
   OLD_MAIN();

   print_separator();
   PRINT_MACRO(BOOST_REGEX_USER_CONFIG);
   PRINT_MACRO(BOOST_REGEX_USE_C_LOCALE);
   PRINT_MACRO(BOOST_REGEX_USE_CPP_LOCALE);
   PRINT_MACRO(BOOST_REGEX_HAS_DLL_RUNTIME);
   PRINT_MACRO(BOOST_REGEX_DYN_LINK);
   PRINT_MACRO(BOOST_REGEX_NO_LIB);
   PRINT_MACRO(BOOST_REGEX_NO_TEMPLATE_SWITCH_MERGE);
   PRINT_MACRO(BOOST_REGEX_NO_W32);
   PRINT_MACRO(BOOST_REGEX_NO_BOOL);
   PRINT_MACRO(BOOST_REGEX_NO_EXTERNAL_TEMPLATES);
   PRINT_MACRO(BOOST_REGEX_NO_FWD);
   PRINT_MACRO(BOOST_REGEX_V3);
   PRINT_MACRO(BOOST_REGEX_HAS_MS_STACK_GUARD);
   PRINT_MACRO(BOOST_REGEX_RECURSIVE);
   PRINT_MACRO(BOOST_REGEX_NON_RECURSIVE);
   PRINT_MACRO(BOOST_REGEX_BLOCKSIZE);
   PRINT_MACRO(BOOST_REGEX_MAX_BLOCKS);
   PRINT_MACRO(BOOST_REGEX_MAX_CACHE_BLOCKS);
   PRINT_MACRO(BOOST_NO_WREGEX);
   PRINT_MACRO(BOOST_REGEX_NO_FILEITER);
   PRINT_MACRO(BOOST_REGEX_STATIC_LINK);
   PRINT_MACRO(BOOST_REGEX_DYN_LINK);
   PRINT_MACRO(BOOST_REGEX_DECL);
   PRINT_MACRO(BOOST_REGEX_CALL);
   PRINT_MACRO(BOOST_REGEX_CCALL);
   PRINT_MACRO(BOOST_REGEX_MAX_STATE_COUNT);
   PRINT_MACRO(BOOST_REGEX_BUGGY_CTYPE_FACET);
   PRINT_MACRO(BOOST_REGEX_MATCH_EXTRA);
   PRINT_MACRO(BOOST_HAS_ICU);
   PRINT_MACRO(BOOST_REGEX_HAS_OTHER_WCHAR_T);

#if defined(BOOST_REGEX_CONFIG_INFO) && !defined(NO_RECURSE)
   print_regex_library_info();
#endif

   return 0;
}



