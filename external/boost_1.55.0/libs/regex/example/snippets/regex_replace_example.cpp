/*
 *
 * Copyright (c) 1998-2002
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_replace_example.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_replace example:
  *                converts a C++ file to syntax highlighted HTML.
  */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iterator>
#include <boost/regex.hpp>
#include <fstream>
#include <iostream>

// purpose:
// takes the contents of a file and transform to
// syntax highlighted code in html format

boost::regex e1, e2;
extern const char* expression_text;
extern const char* format_string;
extern const char* pre_expression;
extern const char* pre_format;
extern const char* header_text;
extern const char* footer_text;

void load_file(std::string& s, std::istream& is)
{
   s.erase();
   if(is.bad()) return;
   s.reserve(static_cast<std::string::size_type>(is.rdbuf()->in_avail()));
   char c;
   while(is.get(c))
   {
      if(s.capacity() == s.size())
         s.reserve(s.capacity() * 3);
      s.append(1, c);
   }
}

int main(int argc, const char** argv)
{
   try{
   e1.assign(expression_text);
   e2.assign(pre_expression);
   for(int i = 1; i < argc; ++i)
   {
      std::cout << "Processing file " << argv[i] << std::endl;
      std::ifstream fs(argv[i]);
      std::string in;
      load_file(in, fs);
      fs.close();
      std::string out_name = std::string(argv[i]) + std::string(".htm");
      std::ofstream os(out_name.c_str());
      os << header_text;
      // strip '<' and '>' first by outputting to a
      // temporary string stream
      std::ostringstream t(std::ios::out | std::ios::binary);
      std::ostream_iterator<char> oi(t);
      boost::regex_replace(oi, in.begin(), in.end(), e2, pre_format, boost::match_default | boost::format_all);
      // then output to final output stream
      // adding syntax highlighting:
      std::string s(t.str());
      std::ostream_iterator<char> out(os);
      boost::regex_replace(out, s.begin(), s.end(), e1, format_string, boost::match_default | boost::format_all);
      os << footer_text;
      os.close();
   }
   }
   catch(...)
   { return -1; }
   return 0;
}

const char* pre_expression = "(<)|(>)|(&)|\\r";
const char* pre_format = "(?1&lt;)(?2&gt;)(?3&amp;)";


const char* expression_text = // preprocessor directives: index 1
                              "(^[[:blank:]]*#(?:[^\\\\\\n]|\\\\[^\\n[:punct:][:word:]]*[\\n[:punct:][:word:]])*)|"
                              // comment: index 2
                              "(//[^\\n]*|/\\*.*?\\*/)|"
                              // literals: index 3
                              "\\<([+-]?(?:(?:0x[[:xdigit:]]+)|(?:(?:[[:digit:]]*\\.)?[[:digit:]]+(?:[eE][+-]?[[:digit:]]+)?))u?(?:(?:int(?:8|16|32|64))|L)?)\\>|"
                              // string literals: index 4
                              "('(?:[^\\\\']|\\\\.)*'|\"(?:[^\\\\\"]|\\\\.)*\")|"
                              // keywords: index 5
                              "\\<(__asm|__cdecl|__declspec|__export|__far16|__fastcall|__fortran|__import"
                              "|__pascal|__rtti|__stdcall|_asm|_cdecl|__except|_export|_far16|_fastcall"
                              "|__finally|_fortran|_import|_pascal|_stdcall|__thread|__try|asm|auto|bool"
                              "|break|case|catch|cdecl|char|class|const|const_cast|continue|default|delete"
                              "|do|double|dynamic_cast|else|enum|explicit|extern|false|float|for|friend|goto"
                              "|if|inline|int|long|mutable|namespace|new|operator|pascal|private|protected"
                              "|public|register|reinterpret_cast|return|short|signed|sizeof|static|static_cast"
                              "|struct|switch|template|this|throw|true|try|typedef|typeid|typename|union|unsigned"
                              "|using|virtual|void|volatile|wchar_t|while)\\>"
                              ;

const char* format_string = "(?1<font color=\"#008040\">$&</font>)"
                            "(?2<I><font color=\"#000080\">$&</font></I>)"
                            "(?3<font color=\"#0000A0\">$&</font>)"
                            "(?4<font color=\"#0000FF\">$&</font>)"
                            "(?5<B>$&</B>)";

const char* header_text = "<HTML>\n<HEAD>\n"
                          "<TITLE>Auto-generated html formated source</TITLE>\n"
                          "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=windows-1252\">\n"
                          "</HEAD>\n"
                          "<BODY LINK=\"#0000ff\" VLINK=\"#800080\" BGCOLOR=\"#ffffff\">\n"
                          "<P> </P>\n<PRE>";

const char* footer_text = "</PRE>\n</BODY>\n\n";












