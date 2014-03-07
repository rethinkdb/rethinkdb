/*
 *
 * Copyright (c) 2003 Dr John Maddock
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include "bcp.hpp"
#include <string>
#include <cstring>
#include <list>
#include <set>
#include <map>
#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;

class fileview;

//
//path operations:
//
int compare_paths(const fs::path& a, const fs::path& b);
inline bool equal_paths(const fs::path& a, const fs::path& b)
{ return compare_paths(a, b) == 0; }

struct path_less
{
   bool operator()(const fs::path& a, const fs::path& b)const
   { return compare_paths(a, b) < 0; }
};

struct license_data
{
   std::set<fs::path, path_less> files;
   std::set<std::string>         authors;
};

class bcp_implementation
   : public bcp_application
{
public:
   bcp_implementation();
   ~bcp_implementation();
   static bool is_source_file(const fs::path& p);
   static bool is_html_file(const fs::path& p);
   static bool is_jam_file(const fs::path& p);
private:
   //
   // the following are the overridden virtuals from the base class:
   //
   void enable_list_mode();
   void enable_summary_list_mode();
   void enable_cvs_mode();
   void enable_svn_mode();
   void enable_unix_lines();
   void enable_scan_mode();
   void enable_license_mode();
   void enable_bsl_convert_mode();
   void enable_bsl_summary_mode();
   void set_boost_path(const char* p);
   void set_destination(const char* p);
   void add_module(const char* p);
   void set_namespace(const char* name);
   void set_namespace_alias(bool);
   void set_namespace_list(bool);

   virtual int run();

   // internal helper functions:
   bool is_binary_file(const fs::path& p);
   void scan_cvs_path(const fs::path& p);
   void scan_svn_path(const fs::path& p);
   void add_path(const fs::path& p);
   void add_directory(const fs::path& p);
   void add_file(const fs::path& p);
   void copy_path(const fs::path& p);
   void add_file_dependencies(const fs::path& p, bool scanfile);
   void add_dependent_lib(const std::string& libname, const fs::path& p, const fileview& view);
   void create_path(const fs::path& p);
   // license code:
   void scan_license(const fs::path& p, const fileview& v);
   void output_license_info();

   std::list<std::string> m_module_list; // the modules to process
   bool m_list_mode;                     // list files only
   bool m_list_summary_mode;             // list file summary only
   bool m_license_mode;                  // generate license information for files listed
   bool m_cvs_mode;                      // check cvs for files
   bool m_svn_mode;                      // check svn for files
   bool m_unix_lines;                    // fix line endings
   bool m_scan_mode;                     // scan non-boost files.
   bool m_bsl_convert_mode;              // try to convert to the BSL
   bool m_bsl_summary_mode;              // summarise BSL issues only
   bool m_namespace_alias;               // make "boost" a namespace alias when doing a namespace rename.
   bool m_list_namespaces;               // list all the top level namespaces found.
   fs::path m_boost_path;                // the path to the boost root
   fs::path m_dest_path;                 // the path to copy to
   std::map<fs::path, bool, path_less>                   m_cvs_paths;                  // valid files under cvs control
   std::set<fs::path, path_less>                         m_copy_paths;                 // list of files to copy
   std::map<int, license_data>                           m_license_data;               // licenses in use
   std::set<fs::path, path_less>                         m_unknown_licenses;           // files with no known license
   std::set<fs::path, path_less>                         m_unknown_authors;            // files with no known copyright/author
   std::set<fs::path, path_less>                         m_can_migrate_to_bsl;         // files that can migrate to the BSL
   std::set<fs::path, path_less>                         m_cannot_migrate_to_bsl;      // files that cannot migrate to the BSL
   std::set<std::string>                                 m_bsl_authors;                // authors giving blanket permission to use the BSL
   std::set<std::string>                                 m_authors_for_bsl_migration;  // authors we need for BSL migration
   std::map<fs::path, std::pair<std::string, std::string>, path_less> 
                                                         m_converted_to_bsl;
   std::map<std::string, std::set<fs::path, path_less> > m_author_data;                // all the authors
   std::map<fs::path, fs::path, path_less>               m_dependencies;               // dependency information
   std::string                                           m_namespace_name;             // namespace rename.
   std::set<std::string>                                 m_lib_names;                  // List of library binary names
   std::map<std::string, fs::path>                       m_top_namespaces;             // List of top level namespace names
};

