/*
 *
 * Copyright (c) 2003 Dr John Maddock
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * This file implements the bcp_implementation virtuals.
 */

#include "bcp_imp.hpp"
#include "licence_info.hpp"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>
#include <stdexcept>
#include <boost/regex.hpp>
#include <string>

bcp_implementation::bcp_implementation()
  : m_list_mode(false), m_list_summary_mode(false), m_license_mode(false), m_cvs_mode(false), 
  m_svn_mode(false), m_unix_lines(false), m_scan_mode(false), m_bsl_convert_mode(false), 
  m_bsl_summary_mode(false), m_namespace_alias(false), m_list_namespaces(false)
{
}

bcp_implementation::~bcp_implementation()
{
}

bcp_application::~bcp_application()
{
}

void bcp_implementation::enable_list_mode()
{
   m_list_mode = true;
}

void bcp_implementation::enable_summary_list_mode()
{
   m_list_mode = true;
   m_list_summary_mode = true;
}

void bcp_implementation::enable_cvs_mode()
{
   m_cvs_mode = true;
}

void bcp_implementation::enable_svn_mode()
{
   m_svn_mode = true;
}

void bcp_implementation::enable_scan_mode()
{
   m_scan_mode = true;
}

void bcp_implementation::enable_license_mode()
{
   m_license_mode = true;
}

void bcp_implementation::enable_bsl_convert_mode()
{
   m_bsl_convert_mode = true;
}

void bcp_implementation::enable_bsl_summary_mode()
{
   m_bsl_summary_mode = true;
}

void bcp_implementation::enable_unix_lines()
{
   m_unix_lines = true;
}

void bcp_implementation::set_boost_path(const char* p)
{
   // Hack to strip trailing slashes from the path 
   m_boost_path = (fs::path(p) / "boost").parent_path(); 
   fs::path check = m_boost_path / "boost" / "version.hpp";
   if(!fs::exists(check))
   {
      std::string s = "The Boost path appears to have been incorrectly set: could not find boost/version.hpp in ";
      s += m_boost_path.string();
      std::runtime_error e(s);
      throw e;
   }
}

void bcp_implementation::set_destination(const char* p)
{
   m_dest_path = fs::path(p);
}

void bcp_implementation::add_module(const char* p)
{
   m_module_list.push_back(p);
}

void bcp_implementation::set_namespace(const char* name)
{
   m_namespace_name = name;
}

void bcp_implementation::set_namespace_alias(bool b)
{
   m_namespace_alias = b;
}

void bcp_implementation::set_namespace_list(bool b)
{
   m_list_namespaces = b;
   m_list_mode = b;
}

fs::path get_short_path(const fs::path& p)
{
   // truncate path no more than "x/y":
   std::string s = p.generic_string();
   std::string::size_type n = s.find('/');
   if(n != std::string::npos)
   {
      n = s.find('/', n+1);
      if(n != std::string::npos)
         s.erase(n);
   }
   return s;
}

int bcp_implementation::run()
{
   //
   // check output path is OK:
   //
   if(!m_list_mode && !m_license_mode && !fs::exists(m_dest_path))
   {
      std::string msg("Destination path does not exist: ");
      msg.append(m_dest_path.string());
      std::runtime_error e(msg);
      boost::throw_exception(e);
   }
   //
   // Check Boost path is OK if it hasn't been checked already:
   //
   if(m_boost_path == "")
   {
      set_boost_path("");
   }
   // start by building a list of permitted files
   // if m_cvs_mode is true:
   if(m_cvs_mode)
   {
      std::cerr << "CAUTION: Boost is no longer in CVS, cvs mode may not work anymore!!!" << std::endl;
      scan_cvs_path(fs::path());
   }
   if(m_svn_mode)
   {
      scan_svn_path(fs::path());
   }
   //
   // if in license mode, try to load more/blanket_permission.txt
   //
   fs::path blanket_permission(m_boost_path / "more" / "blanket-permission.txt");
   if (fs::exists(blanket_permission)) {
     fs::ifstream in(blanket_permission);
     std::string line;
     while (std::getline(in, line)) {
       static const boost::regex e("([^(]+)\\(");
       boost::smatch result;
       if (boost::regex_search(line, result, e))
         m_bsl_authors.insert(format_authors_name(result[1]));
     }
   }

   //
   // scan through modules looking for the equivalent
   // file to add to our list:
   //
   std::list<std::string>::const_iterator i = m_module_list.begin();
   std::list<std::string>::const_iterator j = m_module_list.end();
   while(i != j)
   {
      //
      // convert *i to a path - could be native or portable:
      //
      fs::path module;
      fs::path exmodule;
      module = fs::path(*i);
      exmodule = fs::path(*i + ".hpp");
      
      if(m_scan_mode)
      {
         // in scan mode each module must be a real file:
         add_file_dependencies(module, true);
      }
      else
      {
         int count = 0;
         if(fs::exists(m_boost_path / "tools" / module))
         {
            add_path(fs::path("tools") / module);
            ++count;
         }
         if(fs::exists(m_boost_path / "libs" / module))
         {
            add_path(fs::path("libs") / module);
            ++count;
         }
         if(fs::exists(m_boost_path / "boost" / module))
         {
            add_path(fs::path("boost") / module);
            ++count;
         }
         if(fs::exists(m_boost_path / "boost" / exmodule))
         {
            add_path(fs::path("boost") / exmodule);
            ++count;
         }
         if(fs::exists(m_boost_path / module))
         {
            add_path(module);
            ++count;
         }
      }
      ++i;
   }
   //
   // now perform output:
   //
   if(m_list_namespaces)
   {
      // List the namespaces, in two lists, headers and source files
      // first, then everything else afterwards:
      //
      boost::regex important_file("boost/.*|libs/[^/]*/(?:[^/]*/)?/src/.*");
      std::map<std::string, fs::path>::const_iterator i, j;
      i = m_top_namespaces.begin();
      j = m_top_namespaces.end();
      std::cout << "\n\nThe top level namespaces found for header and source files were:\n";
      while(i != j)
      {
         if(regex_match(i->second.generic_string(), important_file))
            std::cout << i->first << " (from " << i->second << ")" << std::endl;
         ++i;
      }

      i = m_top_namespaces.begin();
      std::cout << "\n\nThe top level namespaces found for all other source files were:\n";
      while(i != j)
      {
         if(!regex_match(i->second.generic_string(), important_file))
            std::cout << i->first << " (from " << i->second << ")" << std::endl;
         ++i;
      }
      return 0;
   }
   std::set<fs::path, path_less>::iterator m, n;
   std::set<fs::path, path_less> short_paths;
   m = m_copy_paths.begin();
   n = m_copy_paths.end();
   if(!m_license_mode)
   {
      while(m != n)
      {
         if(m_list_summary_mode)
         {
            fs::path p = get_short_path(*m);
            if(short_paths.find(p) == short_paths.end())
            {
               short_paths.insert(p);
               std::cout << p.string() << "\n";
            }
         }
         else if(m_list_mode)
            std::cout << m->string() << "\n";
         else
            copy_path(*m);
         ++m;
      }
   }
   else
      output_license_info();
   return 0;
}

pbcp_application bcp_application::create()
{
   pbcp_application result(static_cast<bcp_application*>(new bcp_implementation()));
   return result;
}
