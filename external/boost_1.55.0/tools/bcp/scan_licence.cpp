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
#include <iostream>


const int boost_license_lines = 3;
static const std::string boost_license_text[boost_license_lines] = {
  "Distributed under the Boost Software License, Version 1.0. (See",
  "accompanying file LICENSE_1_0.txt or copy at",
  "http://www.boost.org/LICENSE_1_0.txt)"
};

fileview::const_iterator
context_before_license(const fileview& v, fileview::const_iterator start,
                       int context_lines = 3)
{
  char last_char = '\0';
  while (start != v.begin() && context_lines >= 0) {
    if ((*start == '\r') || (*start == '\n')
        && ((last_char == *start) || ((last_char != '\r') && (last_char != '\n'))))
        --context_lines;

    last_char = *start;
    --start;
  }

  // Unless we hit the beginning, we need to step forward one to start
  // on the next line.
  if (start != v.begin()) ++start;

  return start;
}

fileview::const_iterator
context_after_license(const fileview& v, fileview::const_iterator end,
                      int context_lines = 3)
{
  char last_char = '\0';
  while (end != v.end() && context_lines >= 0) {
    if (*end == '\r' || *end == '\n'
        && (last_char == *end || (last_char != '\r' && last_char != '\n')))
        --context_lines;

    last_char = *end;
    ++end;
  }

  return end;
}

static std::string
find_prefix(const fileview& v, fileview::const_iterator start_of_line)
{
  while (start_of_line != v.begin()
         && *start_of_line != '\n'
         && *start_of_line != '\r')
    --start_of_line;
  if (start_of_line != v.begin())
    ++start_of_line;

  fileview::const_iterator first_noncomment_char = start_of_line;
  while (*first_noncomment_char == '/'
         || *first_noncomment_char == '*'
         || *first_noncomment_char == ' '
         || *first_noncomment_char == '#')
    ++first_noncomment_char;

  return std::string(start_of_line, first_noncomment_char);
}

static std::string 
html_escape(fileview::const_iterator first, fileview::const_iterator last)
{
  std::string result;
  while (first != last) {
    switch (*first) {
    case '<': result += "&lt;"; break;
    case '>': result += "&gt;"; break;
    case '&': result += "&amp;"; break;
    default: result += *first;
    }
    ++first;
  }
  return result;
}

static bool is_non_bsl_license(int index)
{
  return index > 2;
}

void bcp_implementation::scan_license(const fs::path& p, const fileview& v)
{
   std::pair<const license_info*, int> licenses = get_licenses();
   //
   // scan file for all the licenses in the list:
   //
   int license_count = 0;
   int author_count = 0;
   int nonbsl_author_count = 0;
   bool has_non_bsl_license = false;
   fileview::const_iterator start_of_license = v.begin(), 
                            end_of_license = v.end();
   bool start_in_middle_of_line = false;

   for(int i = 0; i < licenses.second; ++i)
   {
      boost::match_results<fileview::const_iterator> m;
      if(boost::regex_search(v.begin(), v.end(), m, licenses.first[i].license_signature))
      {
           start_of_license = m[0].first;
         end_of_license = m[0].second;

         if (is_non_bsl_license(i) && i < licenses.second - 1) 
           has_non_bsl_license = true;

         // add this license to the list:
         m_license_data[i].files.insert(p);
         ++license_count;
         //
         // scan for the associated copyright declarations:
         //
         boost::regex_iterator<const char*> cpy(v.begin(), v.end(), licenses.first[i].copyright_signature);
         boost::regex_iterator<const char*> ecpy;
         while(cpy != ecpy)
         {
#if 0
             // Not dealing with copyrights because we don't have the years
            if ((*cpy)[0].first < start_of_license) 
              start_of_license = (*cpy)[0].first;
            if ((*cpy)[0].second > end_of_license) 
              end_of_license = (*cpy)[0].second;
#endif

            // extract the copy holders as a list:
            std::string author_list = cpy->format(licenses.first[i].copyright_formatter, boost::format_all);
            // now enumerate that list for all the names:
            static const boost::regex author_separator("(?:\\s*,(?!\\s*(?:inc|ltd)\\b)\\s*|\\s+(,\\s*)?(and|&)\\s+)|by\\s+", boost::regex::perl | boost::regex::icase);
            boost::regex_token_iterator<std::string::const_iterator> atr(author_list.begin(), author_list.end(), author_separator, -1);
            boost::regex_token_iterator<std::string::const_iterator> eatr;
            while(atr != eatr)
            {
               // get the reformatted authors name:
               std::string name = format_authors_name(*atr);
               // add to list of authors for this file:
               if(name.size() && name[0] != '-')
               {
                  m_license_data[i].authors.insert(name);
                  // add file to author index:
                  m_author_data[name].insert(p);
                  ++author_count;

                  // If this is not the Boost Software License (license 0), and the author hasn't given 
                  // blanket permission, note this for the report.
                  if (has_non_bsl_license
                      && m_bsl_authors.find(name) == m_bsl_authors.end()) {
                    ++nonbsl_author_count;
                    m_authors_for_bsl_migration.insert(name);
                  }
               }
               ++atr;
            }
            ++cpy;
         }

         while (start_of_license != v.begin()
                && *start_of_license != '\r'
                && *start_of_license != '\n'
                && *start_of_license != '.')
           --start_of_license;

         if (start_of_license != v.begin()) {
           if (*start_of_license == '.')
             start_in_middle_of_line = true;
           ++start_of_license;
         }

         while (end_of_license != v.end()
                && *end_of_license != '\r'
                && *end_of_license != '\n')
           ++end_of_license;
      }
   }
   if(license_count == 0)
      m_unknown_licenses.insert(p);
   if(license_count && !author_count)
      m_unknown_authors.insert(p);

   if (has_non_bsl_license) {
     bool converted = false;
     if (nonbsl_author_count == 0 
         && license_count == 1) {
       // Grab a few lines of context
       fileview::const_iterator context_start = 
         context_before_license(v, start_of_license);
       fileview::const_iterator context_end = 
         context_after_license(v, end_of_license);

       // TBD: For files that aren't C++ code, this will have to
       // change.
       std::string prefix = find_prefix(v, start_of_license);

       // Create enough information to permit manual verification of
       // the correctness of the transformation
       std::string before_conversion = 
         html_escape(context_start, start_of_license);
       before_conversion += "<b>";
       before_conversion += html_escape(start_of_license, end_of_license);
       before_conversion += "</b>";
       before_conversion += html_escape(end_of_license, context_end);

       std::string after_conversion = 
         html_escape(context_start, start_of_license);
       if (start_in_middle_of_line)
         after_conversion += '\n';

       after_conversion += "<b>";
       for (int i = 0; i < boost_license_lines; ++i) {
         if (i > 0) after_conversion += '\n';
         after_conversion += prefix + boost_license_text[i];
       }
       after_conversion += "</b>";
       after_conversion += html_escape(end_of_license, context_end);

       m_converted_to_bsl[p] = 
         std::make_pair(before_conversion, after_conversion);

       // Perform the actual conversion
       if (m_bsl_convert_mode) {
          try{
             std::ofstream out((m_boost_path / p).string().c_str());
            if (!out) {
               std::string msg("Cannot open file for license conversion: ");
               msg += p.string();
               std::runtime_error e(msg);
               boost::throw_exception(e);
            }

            out << std::string(v.begin(), start_of_license);
            if (start_in_middle_of_line)
               out << std::endl;

            for (int j = 0; j < boost_license_lines; ++j) {
               if (j > 0) out << std::endl;
               out << prefix << boost_license_text[j];
            }
            out << std::string(end_of_license, v.end());

            converted = true;
       }
       catch(const std::exception& e)
       {
          std::cerr << e.what() << std::endl;
       }
      }
     }

     if (!converted) {
       if (nonbsl_author_count > 0) m_cannot_migrate_to_bsl.insert(p);
       else m_can_migrate_to_bsl.insert(p);
     }
   }
}

