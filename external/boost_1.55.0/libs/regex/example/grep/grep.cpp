/*
 *
 * Copyright (c) 2004
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/regex.hpp>

#ifdef BOOST_MSVC
#pragma warning(disable:4512 4244)
#endif

#include <boost/program_options.hpp>

namespace po = boost::program_options;

int after_context;
int before_context;
bool print_byte_offset;
bool count_only;
std::string pattern;
bool print_non_matching_files;
bool files_only;
bool print_line_numbers;

boost::regex_constants::syntax_option_type flags = boost::regex_constants::basic;
boost::regex re;
boost::smatch what;
std::string current_file;
int file_count;

void process_stream(std::istream& is)
{
   std::string line;
   int match_found = 0;
   int linenum = 1;
   while(std::getline(is, line))
   {
      bool result = boost::regex_search(line, what, re);
      if(result)
      {
         if(print_non_matching_files)
            return;
         if(files_only)
         {
            std::cout << current_file << std::endl;
            return;
         }
         if(!match_found && !count_only && (file_count > 1))
         {
            std::cout << current_file << ":\n";
         }
         ++match_found;
         if(!count_only)
         {
            if(print_line_numbers)
            {
               std::cout << linenum << ":";
            }
            if(print_byte_offset)
            {
               std::cout << what.position() << ":";
            }
            std::cout << what[0] << std::endl;
         }
      }
      ++linenum;
   }
   if(count_only && match_found)
   {
      std::cout << match_found << " matches found in file " << current_file << std::endl;
   }
   else if(print_non_matching_files && !match_found)
   {
      std::cout << current_file << std::endl;
   }
}

void process_file(const std::string& name)
{
   current_file = name;
   std::ifstream is(name.c_str());
   if(is.bad())
   {
      std::cerr << "Unable to open file " << name << std::endl;
   }
   process_stream(is);
}

int main(int argc, char * argv[])
{
   try{
      po::options_description opts("Options");
      opts.add_options()
         ("help,h", "produce help message") 
         //("after-context,A", po::value<int>(&after_context)->default_value(0), "Print arg  lines  of  trailing  context  after  matching  lines. Places  a  line  containing  --  between  contiguous  groups  of matches.")
         //("before-context,B", po::value<int>(&before_context)->default_value(0), "Print  arg  lines  of  leading  context  before  matching lines. Places  a  line  containing  --  between  contiguous  groups  of matches.")
         //("context,C", po::value<int>(), "Print  arg lines of output context.  Places a line containing -- between contiguous groups of matches.")
         ("byte-offset,b", "Print the byte offset within the input file before each line  of output.")
         ("count,c", "Suppress normal output; instead print a count of matching  lines for  each  input  file.  With the -v, --invert-match option (see below), count non-matching lines.")
         ("extended-regexp,E", "Interpret PATTERN as an POSIX-extended regular expression.")
         ("perl-regexp,P", "Interpret PATTERN as a Perl regular expression.")
         //("regexp,e", po::value<std::string>(&pattern), "Use PATTERN as the pattern; useful to protect patterns beginning with -.")
         ("basic-regexp,G", "Interpret arg as a POSIX-basic regular expression (see below).  This is the default.")
         ("ignore-case,i", "Ignore case distinctions in  both  the  PATTERN  and  the  input files.")
         ("files-without-match,L", "Suppress  normal  output;  instead  print the name of each input file from which no output would normally have been printed.  The scanning will stop on the first match.")
         ("files-with-matches,l", "Suppress  normal  output;  instead  print the name of each input file from which output would normally have  been  printed.   The scanning will stop on the first match.")
         ("line-number,n", "Prefix each line of output with the line number within its input file.")
         ;
      // Hidden options, will be allowed both on command line and
      // in config file, but will not be shown to the user.
      po::options_description hidden("Hidden options");
      hidden.add_options()
         ("input-file", po::value< std::vector<std::string> >(), "input file")
         ("input-pattern", po::value< std::string >(), "input file")
         ;

      po::options_description cmdline_options;
      cmdline_options.add(opts).add(hidden);

      po::positional_options_description p;
      p.add("input-pattern", 1);
      p.add("input-file", -1);

      po::variables_map vm;
      po::store(po::command_line_parser(argc, argv).options(cmdline_options)/*.options(hidden)*/.positional(p).run(), vm);
      po::notify(vm);

      if (vm.count("help")) 
      {
         std::cout << opts << "\n";
         return 0;
      }
      if (vm.count("context")) 
      {
         after_context = vm["context"].as< int >();
         before_context = after_context;
      }
      if(vm.count("extended-regexp"))
      {
         flags = boost::regex_constants::extended;
      }
      if(vm.count("basic-regexp"))
      {
         flags = boost::regex_constants::basic;
      }
      if(vm.count("perl-regexp"))
      {
         flags = boost::regex_constants::perl;
      }
      if(vm.count("ignore-case"))
      {
         flags |= boost::regex_constants::icase;
      }
      if(vm.count("byte-offset"))
      {
         print_byte_offset = true;
      }
      if(vm.count("count"))
      {
         count_only = true;
      }
      if(vm.count("files-without-match"))
      {
         print_non_matching_files = true;
      }
      if(vm.count("files-with-matches"))
      {
         files_only = true;
      }
      if(vm.count("line-number"))
      {
         print_line_numbers = true;
      }
      if(vm.count("input-pattern"))
      {
         pattern = vm["input-pattern"].as< std::string >();
         re.assign(pattern, flags);
      }
      else
      {
         std::cerr << "No pattern specified" << std::endl;
         return 1;
      }
      if (vm.count("input-file"))
      {
         const std::vector<std::string>& files = vm["input-file"].as< std::vector<std::string> >();
         file_count = files.size();
         for(std::vector<std::string>::const_iterator i = files.begin(); i != files.end(); ++i)
         {
            process_file(*i);
         }
      }
      else
      {
         // no input files, scan stdin instead:
         process_stream(std::cin);
      }

   }
   catch(const std::exception& e)
   {
      std::cerr << e.what() << std::endl;
   }

   return 0;
}

