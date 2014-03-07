/*
 *
 * Copyright (c) 2003 Dr John Maddock
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * This file implements the following:
 *    void bcp_implementation::is_source_file(const fs::path& p)
 *    void bcp_implementation::is_html_file(const fs::path& p)
 *    void bcp_implementation::is_binary_file(const fs::path& p)
 */

#include "bcp_imp.hpp"
#include <boost/regex.hpp>

bool bcp_implementation::is_source_file(const fs::path& p)
{
   static const boost::regex e(
      ".*\\."
      "(?:"
         "c|cxx|h|hxx|inc|inl|.?pp|yy?"
      ")", 
      boost::regex::perl | boost::regex::icase
      );
   return boost::regex_match(p.filename().generic_string(), e);
}

bool bcp_implementation::is_html_file(const fs::path& p)
{
   static const boost::regex e(
      ".*\\."
      "(?:"
         "html?|css"
      ")"
      );
   return boost::regex_match(p.filename().generic_string(), e);
}

bool bcp_implementation::is_binary_file(const fs::path& p)
{
   if(m_cvs_mode || m_svn_mode)
   {
      std::map<fs::path, bool, path_less>::iterator pos = m_cvs_paths.find(p);
      if(pos != m_cvs_paths.end()) return pos->second;
   }
   static const boost::regex e(
      ".*\\."
      "(?:"
         "c|cxx|cpp|h|hxx|hpp|inc|html?|css|mak|in"
      ")"
      "|"
      "(Jamfile|makefile|configure)",
      boost::regex::perl | boost::regex::icase);
   return !boost::regex_match(p.leaf().generic_string(), e);

}

bool bcp_implementation::is_jam_file(const fs::path& p)
{
   static const boost::regex e(
      ".*\\."
      "(?:"
         "jam|v2"
      ")"
      "|"
      "(Jamfile|Jamroot)\\.?", 
      boost::regex::perl | boost::regex::icase
      );
   return boost::regex_match(p.filename().generic_string(), e);
}

