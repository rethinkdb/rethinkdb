/*
 *
 * Copyright (c) 2009 Dr John Maddock
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * This file implements the following:
 *    void bcp_implementation::add_path(const fs::path& p)
 *    void bcp_implementation::add_directory(const fs::path& p)
 *    void bcp_implementation::add_file(const fs::path& p)
 *    void bcp_implementation::add_dependent_lib(const std::string& libname, const fs::path& p, const fileview& view)
 */

#include "bcp_imp.hpp"
#include "fileview.hpp"
#include <boost/regex.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>
#include <iostream>

//
// This file contains the code required to work out whether the source/header file being scanned
// is actually dependent upon some library's source code or not.
//

static std::map<std::string, boost::regex> scanner;

static std::map<std::string, std::set<std::string> > free_function_names;
static std::map<std::string, std::set<std::string> > class_names;
static std::map<std::string, std::set<std::string> > variable_names;

static void init_library_scanner(const fs::path& p, bool cvs_mode, const std::string& libname, bool recurse = false)
{
   /*
   if(free_function_names.count(libname) == 0)
   {
      free_function_names[libname] = "[\\x0]";
      class_names[libname] = "[\\x0]";
      variable_names[libname] = "[\\x0]";
   }
   */
   //
   // Don't add files created by build system:
   //
   if((p.leaf() == "bin") || (p.leaf() == "bin-stage"))
      return; 
   //
   // Don't add version control directories:
   //
   if((p.leaf() == "CVS") || (p.leaf() == ".svn"))
      return; 
   //
   // don't add directories not under version control:
   //
   if(cvs_mode && !fs::exists(p / "CVS/Entries"))
      return;
   if(cvs_mode && !fs::exists(p / ".svn/entries"))
      return;
   //
   // Enumerate files and directories:
   //
   fs::directory_iterator i(p);
   fs::directory_iterator j;
   while(i != j)
   {
      if(fs::is_directory(*i))
         init_library_scanner(*i, cvs_mode, libname, true);
      if(bcp_implementation::is_source_file(*i))
      {
         static boost::regex function_scanner(
            "(?|"                       // Branch reset group
               "(?:\\<\\w+\\>[^>;{},:]*)"  // Return type
               "(?:"
                  "(\\<\\w+\\>)"           // Maybe class name
                  "\\s*"
                  "(?:<[^>;{]*>)?"         // Maybe template specialisation
                  "::\\s*)?"
               "(\\<(?!throw|if|while|for|catch)\\w+\\>)" // function name
               "\\s*"
               "\\("
                  "[^\\(\\);{}]*"          // argument list
               "\\)"
               "\\s*(?:BOOST[_A-Z]+\\s*)?"
               "\\{"                       // start of definition
            "|"
               "(\\<\\w+\\>)"              // Maybe class name
               "\\s*"
               "(?:<[^>;{]*>)?"            // Maybe template specialisation
               "::\\s*"
               "~?\\1"                     // function name, same as class name
               "\\s*"
               "\\("
                  "[^\\(\\);{}]*"          // argument list
               "\\)"
               "\\s*(?:BOOST[_A-Z]+\\s*)?"
               "\\{"                       // start of definition
            ")"                            // end branch reset
            );
         fileview view(*i);
         boost::regex_iterator<const char*> a(view.begin(), view.end(), function_scanner);
         boost::regex_iterator<const char*> b;
         while(a != b)
         {
            if((*a)[1].matched)
            {
               std::string n = a->str(1);
               class_names[libname].insert(n);
            }
            else
            {
               std::string n = a->str(2);
               free_function_names[libname].insert(n);
            }
            ++a;
         }
      }
      ++i;
   }

   if(recurse == false)
   {
      //
      // Build the regular expressions:
      //
      const char* e1 = 
         "^(?>[[:blank:]]*)(?!#)[^;{}\\r\\n]*"
         "(?|"
         "(?:class|struct)[^:;{}#]*"
         "(";
      // list of class names goes here...
      const char* e2 = 
         ")\\s*(?:<[^;{>]*>\\s*)?(?::[^;{]*)?\\{"
         "|"
         "\\<(?!return)\\w+\\>[^:;{}#=<>!~%.\\w]*(";
         // List of function names goes here...
      const char* e3 = 
         ")\\s*\\([^;()]*\\)\\s*(?:BOOST[_A-Z]+\\s*)?;)";

      std::string class_name_list;
      std::set<std::string>::const_iterator i = class_names[libname].begin(), j = class_names[libname].end();
      if(i != j)
      {
         class_name_list = *i;
         ++i;
         while(i != j)
         {
            class_name_list += "|" + *i;
            ++i;
         }
      }
      else
      {
         class_name_list = "[\\x0]";
      }
      std::string function_name_list;
      i = free_function_names[libname].begin();
      j = free_function_names[libname].end();
      if(i != j)
      {
         function_name_list = *i;
         ++i;
         while(i != j)
         {
            function_name_list += "|" + *i;
            ++i;
         }
      }
      else
      {
         function_name_list = "[\\x0]";
      }

      scanner[libname] = boost::regex(e1 + class_name_list + e2 + function_name_list + e3);
   }
}

void bcp_implementation::add_dependent_lib(const std::string& libname, const fs::path& p, const fileview& view)
{
   //
   // if the boost library libname has source associated with it
   // then add the source to our list:
   //
   if(fs::exists(m_boost_path / "libs" / libname / "src"))
   {
      if(!m_dependencies.count(fs::path("libs") / libname / "src")) 
      {
         if(scanner.count(libname) == 0)
            init_library_scanner(m_boost_path / "libs" / libname / "src", m_cvs_mode, libname);
         boost::cmatch what;
         if(regex_search(view.begin(), view.end(), what, scanner[libname]))
         {
            std::cout << "INFO: tracking source dependencies of library " << libname
               << " due to presence of \"" << what << "\" in file " << p << std::endl;
            //std::cout << "Full text match was: " << what << std::endl;
            m_dependencies[fs::path("libs") / libname / "src"] = p; // set up dependency tree
            add_path(fs::path("libs") / libname / "src");

            if(fs::exists(m_boost_path / "libs" / libname / "build"))
            {
               if(!m_dependencies.count(fs::path("libs") / libname / "build")) 
               {
                  m_dependencies[fs::path("libs") / libname / "build"] = p; // set up dependency tree
                  add_path(fs::path("libs") / libname / "build");
                  //m_dependencies[fs::path("boost-build.jam")] = p;
                  //add_path(fs::path("boost-build.jam"));
                  m_dependencies[fs::path("Jamroot")] = p;
                  add_path(fs::path("Jamroot"));
                  //m_dependencies[fs::path("tools/build")] = p;
                  //add_path(fs::path("tools/build"));
               }
            }
         }
      }
   }
}
