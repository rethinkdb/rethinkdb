// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2010-2013 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2012-2013 Adam Wulkiewicz, Lodz, Poland.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//
#ifndef FILE_TO_STRING_HPP
#define FILE_TO_STRING_HPP


#include <string>
#include <fstream>


inline std::string file_to_string(std::string const& filename)
{
    std::string result;

    std::ifstream cpp_file(filename.c_str());
    if (cpp_file.is_open())
    {
        while (! cpp_file.eof() )
        {
            std::string line;
            std::getline(cpp_file, line);
            result += line + "\n";
        }
    }
    return result;
}


#endif // FILE_TO_STRING_HPP
