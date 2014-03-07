/*
 *
 * Copyright (c) 2003 Dr John Maddock
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include "licence_info.hpp"
#include "bcp_imp.hpp"
#include "fileview.hpp"
#include <fstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/throw_exception.hpp>

//
// split_path is a small helper for outputting a path name, 
// complete with a link to that path:
//
struct split_path
{
   const fs::path& root;
   const fs::path& file;
   split_path(const fs::path& r, const fs::path& f)
      : root(r), file(f){}
private:
   split_path& operator=(const split_path&);
};

std::ostream& operator << (std::ostream& os, const split_path& p)
{
   os << "<a href=\"" << (p.root / p.file).string() << "\">" << p.file.string() << "</a>";
   return os;
}

std::string make_link_target(const std::string& s)
{
   // convert an arbitrary string into something suitable
   // for an <a> name:
   std::string result;
   for(unsigned i = 0; i < s.size(); ++i)
   {
      result.append(1, static_cast<std::string::value_type>(std::isalnum(s[i]) ? s[i] : '_'));
   }
   return result;
}


void bcp_implementation::output_license_info()
{
   std::pair<const license_info*, int> licenses = get_licenses();

   std::map<int, license_data>::const_iterator i, j;
   i = m_license_data.begin();
   j = m_license_data.end();

   std::ofstream os(m_dest_path.string().c_str());
   if(!os)
   {
      std::string msg("Error opening ");
      msg += m_dest_path.string();
      msg += " for output.";
      std::runtime_error e(msg);
      boost::throw_exception(e);
   }
   os << 
      "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n"
      "<html>\n"
      "<head>\n"
      "<title>Boost Licence Dependency Information";
   if(m_module_list.size() == 1)
   {
      os << " for " << *(m_module_list.begin());
   }
   os << 
      "</title>\n"
      "</head>\n"
      "<body>\n"
      "<H1>Boost Licence Dependency Information";
   if(m_module_list.size() == 1)
   {
      os << " for " << *(m_module_list.begin());
   }
   os << 
      "</H1>\n"
      "<H2>Contents</h2>\n"
      "<pre><a href=\"#input\">Input Information</a>\n";
   if(!m_bsl_summary_mode)
      os << "<a href=\"#summary\">Licence Summary</a>\n";
   os << "<a href=\"#details\">Licence Details</a>\n";

   while(i != j)
   {
      // title:
      os << "   <A href=\"#" << make_link_target(licenses.first[i->first].license_name) 
         << "\">" << licenses.first[i->first].license_name << "</a>\n";
      ++i;
   }

   os << "<a href=\"#files\">Files with no recognised license</a>\n"
      "<a href=\"#authors\">Files with no recognised copyright holder</a>\n";
   if(!m_bsl_summary_mode)
   {
      os <<
      "Moving to the Boost Software License...\n"
      "  <a href=\"#bsl-converted\">Files that can be automatically converted to the Boost Software License</a>\n"
      "  <a href=\"#to-bsl\">Files that can be manually converted to the Boost Software License</a>\n"
      "  <a href=\"#not-to-bsl\">Files that can <b>NOT</b> be moved to the Boost Software License</a>\n"
      "  <a href=\"#need-bsl-authors\">Authors we need to move to the Boost Software License</a>\n"
      "<a href=\"#copyright\">Copyright Holder Information</a>\n";
   }
   os << 
      "<a href=\"#depend\">File Dependency Information</a>\n"
      "</pre>";

   //
   // input Information:
   //
   os << "<a name=\"input\"></a><h2>Input Information</h2>\n";
   if(m_scan_mode)
      os << "<P>The following files were scanned for boost dependencies:<BR>";
   else
      os << "<P>The following Boost modules were checked:<BR>";

   std::list<std::string>::const_iterator si = m_module_list.begin();
   std::list<std::string>::const_iterator sj = m_module_list.end();
   while(si != sj)
   {
      os << *si << "<BR>";
      ++si;
   }
   os << "</p><p>The Boost path was: <code>" << m_boost_path.string() << "</code></P>";
   //
   // extract the boost version number from the boost directory tree, 
   // not from this app (which may have been built from a previous
   // version):
   //
   fileview version_file(m_boost_path / "boost/version.hpp");
   static const boost::regex version_regex(
      "^[[:blank:]]*#[[:blank:]]*define[[:blank:]]+BOOST_VERSION[[:blank:]]+(\\d+)");
   boost::cmatch what;
   if(boost::regex_search(version_file.begin(), version_file.end(), what, version_regex))
   {
      int version = boost::lexical_cast<int>(what.str(1));
      os << "<p>The Boost version is: " << version / 100000 << "." << version / 100 % 1000 << "." << version % 100 << "</P>\n";
   }

   //
   // output each license:
   //
   i = m_license_data.begin();
   j = m_license_data.end();
   if(!m_bsl_summary_mode)
   {
      //
      // start with the summary:
      //
      os << "<a name=\"summary\"></a><h2>Licence Summary</h2>\n";
      while(i != j)
      {
         // title:
         os << 
            "<H3>" << licenses.first[i->first].license_name << "</H3>\n";
         // license text:
         os << "<BLOCKQUOTE>" << licenses.first[i->first].license_text << "</BLOCKQUOTE>";
         // Copyright holders:
         os << "<P>This license is used by " << i->second.authors.size() 
            << " authors and " << i->second.files.size() 
            << " files <a href=\"#" << make_link_target(licenses.first[i->first].license_name) << "\">(see details)</a>";
         os << "</P></BLOCKQUOTE>\n";
         ++i;
      }
   }
   //
   // and now the details:
   //
   i = m_license_data.begin();
   j = m_license_data.end();
   int license_index = 0;
   os << "<a name=\"details\"></a><h2>Licence Details</h2>\n";
   while(i != j)
   {
      // title:
      os << 
         "<H3><A name=\"" << make_link_target(licenses.first[i->first].license_name) 
         << "\"></a>" << licenses.first[i->first].license_name << "</H3>\n";
      // license text:
      os << "<BLOCKQUOTE>" << licenses.first[i->first].license_text << "</BLOCKQUOTE>";
      if(!m_bsl_summary_mode || (license_index >= 3))
      {
         // Copyright holders:
         os << "<P>This license is used by the following " << i->second.authors.size() << " copyright holders:</P>\n<BLOCKQUOTE><P>";
         std::set<std::string>::const_iterator x, y;
         x = i->second.authors.begin();
         y = i->second.authors.end();
         while(x != y)
         {
            os << *x << "<BR>\n";
            ++x;
         }
         os << "</P></BLOCKQUOTE>\n";
         // Files using this license:
         os << "<P>This license applies to the following " << i->second.files.size() << " files:</P>\n<BLOCKQUOTE><P>";
         std::set<fs::path, path_less>::const_iterator m, n;
         m = i->second.files.begin();
         n = i->second.files.end();
         while(m != n)
         {
            os << split_path(m_boost_path, *m) << "<br>\n";
            ++m;
         }
         os << "</P></BLOCKQUOTE>\n";
      }
      else
      {
         os << "<P>This license is used by " << i->second.authors.size() << " authors (list omitted for brevity).</P>\n";
         os << "<P>This license applies to " << i->second.files.size() << " files (list omitted for brevity).</P>\n";
      }
      ++license_index;
      ++i;
   }
   //
   // Output list of files not found to be under license control:
   //
   os << "<h2><a name=\"files\"></a>Files With No Recognisable Licence</h2>\n"
      "<P>The following " << m_unknown_licenses.size() << " files had no recognisable license information:</P><BLOCKQUOTE><P>\n";
   std::set<fs::path, path_less>::const_iterator i2, j2;
   i2 = m_unknown_licenses.begin();
   j2 = m_unknown_licenses.end();
   while(i2 != j2)
   {
      os << split_path(m_boost_path, *i2) << "<br>\n";
      ++i2;
   }
   os << "</p></BLOCKQUOTE>";
   //
   // Output list of files with no found copyright holder:
   //
   os << "<h2><a name=\"authors\"></a>Files With No Recognisable Copyright Holder</h2>\n"
      "<P>The following " << m_unknown_authors.size() << " files had no recognisable copyright holder:</P>\n<BLOCKQUOTE><P>";
   i2 = m_unknown_authors.begin();
   j2 = m_unknown_authors.end();
   while(i2 != j2)
   {
      os << split_path(m_boost_path, *i2) << "<br>\n";
      ++i2;
   }
   os << "</p></BLOCKQUOTE>";
   if(!m_bsl_summary_mode)
   {
      //
      // Output list of files that have been moved over to the Boost
      // Software License, along with enough information for human
      // verification.
      //
      os << "<h2><a name=\"bsl-converted\"></a>Files that can be automatically converted to the Boost Software License</h2>\n"
         << "<P>The following " << m_converted_to_bsl.size() << " files can be automatically converted to the Boost Software License, but require manual verification before they can be committed to CVS:</P>\n";
      if (!m_converted_to_bsl.empty()) 
      {
         typedef std::map<fs::path, std::pair<std::string, std::string>, path_less>
            ::const_iterator conv_iterator;
         conv_iterator i = m_converted_to_bsl.begin(), 
                        ie = m_converted_to_bsl.end();
         int file_num = 1;
         while (i != ie) 
         {
            os << "<P>[" << file_num << "] File: <tt>" << split_path(m_boost_path, i->first) 
               << "</tt><br>\n<table border=\"1\">\n  <tr>\n    <td><pre>" 
               << i->second.first << "</pre></td>\n    <td><pre>"
               << i->second.second << "</pre></td>\n  </tr>\n</table>\n";
            ++i;
            ++file_num;
         }
      }
      //
      // Output list of files that could be moved over to the Boost Software License
      //
      os << "<h2><a name=\"to-bsl\"></a>Files that could be converted to the Boost Software License</h2>\n"
      "<P>The following " << m_can_migrate_to_bsl.size() << " files could be manually converted to the Boost Software License, but have not yet been:</P>\n<BLOCKQUOTE><P>";
      i2 = m_can_migrate_to_bsl.begin();
      j2 = m_can_migrate_to_bsl.end();
      while(i2 != j2)
      {
         os << split_path(m_boost_path, *i2) << "<br>\n";
         ++i2;
      }
      os << "</p></BLOCKQUOTE>";
      //
      // Output list of files that can not be moved over to the Boost Software License
      //
      os << "<h2><a name=\"not-to-bsl\"></a>Files that can NOT be converted to the Boost Software License</h2>\n"
      "<P>The following " << m_cannot_migrate_to_bsl.size() << " files cannot be converted to the Boost Software License because we need the permission of more authors:</P>\n<BLOCKQUOTE><P>";
      i2 = m_cannot_migrate_to_bsl.begin();
      j2 = m_cannot_migrate_to_bsl.end();
      while(i2 != j2)
      {
         os << split_path(m_boost_path, *i2) << "<br>\n";
         ++i2;
      }
      os << "</p></BLOCKQUOTE>";
      //
      // Output list of authors that we need permission for to move to the BSL
      //
      os << "<h2><a name=\"need-bsl-authors\"></a>Authors we need for the BSL</h2>\n"
         "<P>Permission of the following authors is needed before we can convert to the Boost Software License. The list of authors that have given their permission is contained in <code>more/blanket-permission.txt</code>.</P>\n<BLOCKQUOTE><P>";
      std::copy(m_authors_for_bsl_migration.begin(), m_authors_for_bsl_migration.end(),
               std::ostream_iterator<std::string>(os, "<br>\n"));
      os << "</p></BLOCKQUOTE>";
      //
      // output a table of copyright information:
      //
      os << "<H2><a name=\"copyright\"></a>Copyright Holder Information</H2><table border=\"1\">\n";
      std::map<std::string, std::set<fs::path, path_less> >::const_iterator ad, ead; 
      ad = m_author_data.begin();
      ead = m_author_data.end();
      while(ad != ead)
      {
         os << "<tr><td>" << ad->first << "</td><td>";
         std::set<fs::path, path_less>::const_iterator fi, efi;
         fi = ad->second.begin();
         efi = ad->second.end();
         while(fi != efi)
         {
            os << split_path(m_boost_path, *fi) << " ";
            ++fi;
         }
         os << "</td></tr>\n";
         ++ad;
      }
      os << "</table>\n";
   }

   //
   // output file dependency information:
   //
   os << "<H2><a name=\"depend\"></a>File Dependency Information</H2><BLOCKQUOTE><pre>\n";
   std::map<fs::path, fs::path, path_less>::const_iterator dep, last_dep;
   std::set<fs::path, path_less>::const_iterator fi, efi;
   fi = m_copy_paths.begin();
   efi = m_copy_paths.end();
   // if in summary mode, just figure out the "bad" files and print those only:
   std::set<fs::path, path_less> bad_paths;
   if(m_bsl_summary_mode)
   {
      bad_paths.insert(m_unknown_licenses.begin(), m_unknown_licenses.end());
      bad_paths.insert(m_unknown_authors.begin(), m_unknown_authors.end());
      bad_paths.insert(m_can_migrate_to_bsl.begin(), m_can_migrate_to_bsl.end());
      bad_paths.insert(m_cannot_migrate_to_bsl.begin(), m_cannot_migrate_to_bsl.end());
      typedef std::map<fs::path, std::pair<std::string, std::string>, path_less>
         ::const_iterator conv_iterator;
      conv_iterator i = m_converted_to_bsl.begin(), 
                     ie = m_converted_to_bsl.end();
      while(i != ie)
      {
         bad_paths.insert(i->first);
         ++i;
      }
      fi = bad_paths.begin();
      efi = bad_paths.end();
      os << "<P>For brevity, only files not under the BSL are shown</P>\n";
   }
   while(fi != efi)
   {
      os << split_path(m_boost_path, *fi);
      dep = m_dependencies.find(*fi);
      last_dep = m_dependencies.end();
      std::set<fs::path, path_less> seen_deps;
      if (dep != last_dep) 
        while(true)
          {
            os << " -> ";
            if(fs::exists(m_boost_path / dep->second))
              os << split_path(m_boost_path, dep->second);
            else if(fs::exists(dep->second))
              os << split_path(fs::path(), dep->second);
            else
              os << dep->second.string();
            if(seen_deps.find(dep->second) != seen_deps.end())
              {
                os << " <I>(Circular dependency!)</I>";
                break; // circular dependency!!!
              }
            seen_deps.insert(dep->second);
            last_dep = dep;
            dep = m_dependencies.find(dep->second);
            if((dep == m_dependencies.end()) || (0 == compare_paths(dep->second, last_dep->second)))
              break;
          }
      os << "\n";
      ++fi;
   }
   os << "</pre></BLOCKQUOTE>\n";

   os << "</body></html>\n";

   if(!os)
   {
      std::string msg("Error writing to ");
      msg += m_dest_path.string();
      msg += ".";
      std::runtime_error e(msg);
      boost::throw_exception(e);
   }

}
