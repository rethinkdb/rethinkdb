// Copyright Frank Mori Hess 2009.
//
// Quick hack to extract code snippets from example programs, so
// they can be included into boostbook.
//
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

int main(int argc, const char *argv[])
{
  if(argc < 3)
  {
    std::cerr << "Too few arguments: need output directory and input file name(s).\n";
    return -1;
  }
  static const std::string output_directory = argv[1];
  static const int num_files = argc - 2;
  int i;
  for(i = 0; i < num_files; ++i)
  {
    const std::string file_name = argv[2 + i];
    std::cout << "opening file: " << file_name << std::endl;
    std::ifstream infile(file_name.c_str());
    bool inside_snippet = false;
    std::ofstream snippet_out_file;
    while(infile.good())
    {
      std::string line;
      getline(infile, line);
      if(infile.bad()) break;
      if(inside_snippet)
      {
        size_t snippet_end_pos = line.find("//]");
        if(snippet_end_pos == std::string::npos)
        {
          snippet_out_file << line << "\n";
        }else
        {
          snippet_out_file << "]]></code>";
          inside_snippet = false;
          std::cout << "done.\n";
          continue;
        }
      }else
      {
        size_t snippet_start_pos = line.find("//[");
        if(snippet_start_pos == std::string::npos)
        {
          continue;
        }else
        {
          inside_snippet = true;
          std::string snippet_name = line.substr(snippet_start_pos + 3);
          std::istringstream snippet_stream(snippet_name);
          snippet_stream >> snippet_name;
          if(snippet_name == "")
          {
            throw std::runtime_error("failed to obtain snippet name");
          }
          snippet_out_file.close();
          snippet_out_file.clear();
          snippet_out_file.open(std::string(output_directory + "/" + snippet_name + ".xml").c_str());
          snippet_out_file << "<!-- Code snippet \"" << snippet_name <<
            "\" extracted from \"" << file_name << "\" by snippet_extractor.\n" <<
            "--><code><![CDATA[";
          std::cout << "processing snippet \"" << snippet_name << "\"... ";
          continue;
        }
      }
    }
  }
  return 0;
}
