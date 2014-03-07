/*
 *
 * Copyright (c) 2003 Dr John Maddock
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * This file implements path comparisons
 */


#include "bcp_imp.hpp"
#include <cctype>

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std{
   using ::tolower;
}
#endif


int compare_paths(const fs::path& a, const fs::path& b)
{
   const std::string& as = a.generic_string();
   const std::string& bs = b.generic_string();
   std::string::const_iterator i, j, k, l;
   i = as.begin();
   j = as.end();
   k = bs.begin();
   l = bs.end();
   while(i != j)
   {
      if(k == l)
         return -1;
      int r = std::tolower(*i) - std::tolower(*k);
      if(r) return r;
      ++i;
      ++k;
   }
   return (k == l) ? 0 : 1;
}
