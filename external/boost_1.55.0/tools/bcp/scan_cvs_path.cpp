/*
 *
 * Copyright (c) 2003-7 John Maddock
 * Copyright (c) 2007 Bjorn Roald
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * This file implements the following:
 *    void bcp_implementation::scan_cvs_path(const fs::path& p)
 */



#include "bcp_imp.hpp"
#include "fileview.hpp"
#include <boost/regex.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/detail/workaround.hpp>
#include <iostream>

void bcp_implementation::scan_cvs_path(const fs::path& p)
{
   //
   // scan through the cvs admin files to build a list
   // of all the files under cvs version control
   // and whether they are text or binary:
   //
   static const char* file_list[] = { "CVS/Entries", "CVS/Entries.Log" };
   static const boost::regex file_expression("^(?:A\\s+)?/([^/\\n]+)/[^/\\n]*/[^/\\n]*/[^k/\\n]*(kb[^/\\n]*)?/[^/\\n]*");
   static const boost::regex dir_expression("^(?:A\\s+)?D/([^/\\n]+)/");
   static const int file_subs[] = {1,2,};

   for(std::size_t entry = 0; entry < sizeof(file_list)/sizeof(file_list[0]); ++entry)
   {
      fs::path entries(m_boost_path / p / file_list[entry]);
      if(fs::exists(entries))
      {
         fileview view(entries);
         boost::regex_token_iterator<const char*> i(view.begin(), view.end(), dir_expression, 1);
         boost::regex_token_iterator<const char*> j;
         while(i != j)
         {
            fs::path recursion_dir(p / i->str());
            scan_cvs_path(recursion_dir);
            ++i;
         }
   #if BOOST_WORKAROUND(BOOST_MSVC, < 1300) || BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x570))
         std::vector<int> v(file_subs, file_subs + 2);
         i = boost::regex_token_iterator<const char*>(view.begin(), view.end(), file_expression, v);
   #else
         i = boost::regex_token_iterator<const char*>(view.begin(), view.end(), file_expression, file_subs);
   #endif
         while(i != j)
         {
            fs::path file = p / i->str();
            ++i;
            bool binary = i->length() ? true : false;
            ++i;
            m_cvs_paths[file] = binary;
         }

      }
   }
}

void bcp_implementation::scan_svn_path(const fs::path& p)
{
   //
   // scan through the svn entries files to build a list
   // of all the files under svn version control
   // and whether they are text or binary:
   //
   static const boost::regex entry_expression("^\\f([^\\f]*)");
   static const boost::regex entry_line_expression("\\n[[:blank:]]*([^\\n]*)");
   //    static const boost::regex 
   //      mime_type_expression("\\nsvn:mime-type\\nV [[digit]]*\\n([^/]*)[^\\n]*");

   fs::path entries(m_boost_path / p / ".svn" / "entries");
   if(fs::exists(entries))
   {
      fileview view(entries);
      boost::cregex_token_iterator 
         i(view.begin(), view.end(), entry_expression, 1), j;

      while(i != j) // entries
      {
         std::string entr = i->str();
         boost::sregex_token_iterator
            atr_it(entr.begin(), entr.end(), entry_line_expression, 1), atr_j;       

         if(atr_it != atr_j) 
         {
            std::string name = atr_it->str(); // name of file or directory
            fs::path fpath = p / name;
            if(++atr_it != atr_j)
            {
               std::string kind = atr_it->str();
               if(kind == "file")
               {
                  // find if binary, we asume text unless mime type is 
                  // set in property file
                  bool binary = false;  // 

                  // skip some lines    type                   | example
                  if( ++atr_it != atr_j  &&  // revnum         |    
                     ++atr_it != atr_j  &&  // url            |
                     ++atr_it != atr_j  &&  // repos          |
                     ++atr_it != atr_j  &&  // scedule attr   |
                     ++atr_it != atr_j  &&  // text timestamp | 2007-09-02T...
                     ++atr_it != atr_j  &&  // checksum       | 58f4bfa7860...
                     ++atr_it != atr_j  &&  // cmt_date       | 2007-05-09T...
                     ++atr_it != atr_j  &&  // cmt_rev        | 37654
                     ++atr_it != atr_j  &&  // cmt_author     | dgregor
                     ++atr_it != atr_j )    // has_props      | has-props
                  {
                     if(atr_it->str() == "has-props")
                     {
                        // we  need to check properties file for mime-type
                        // that does not start with "text/", if found file is binary
                        fs::path properties(m_boost_path / p / ".svn" / "prop-base" 
                           / (name + ".svn-base") );
                        if(fs::exists(properties))
                        {
                           fileview prop(properties);

                           static const boost::regex mime_type(
                              "svn:mime-type[[:blank:]]*(?:\\n|\\r|\\r\\n)[^\\r\\n]*(?:\\n|\\r|\\r\\n)[[:blank:]]*text/");
                           binary = regex_search(prop.begin(), prop.end(), mime_type) ? false : true;
                        }
                     }
                  }
                  m_cvs_paths[fpath] = binary;
               }  // kind == "file"
               else if(kind == "dir")
               {
                  scan_svn_path(fpath); // recursion for directory entries
               }
               //       else
               //         std::cerr << "WARNING: unknown entry kind for entry " << name 
               //              << "in " << entries << std::endl;  
            }
         }
         ++i;
      } // while
   } 
}
