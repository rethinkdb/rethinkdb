// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2010-2013 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2012-2013 Adam Wulkiewicz, Lodz, Poland.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//
#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP


#include <string>
#include <vector>



struct configuration
{
    // To transfer e.g. c:/_svn/boost/trunk/boost/geometry/algorithms/area.hpp
    // to #include <boost/geometry/...>
    // We need to find the position where the include path should start,
    // so fill out "boost" here, or "boost/geometry" (it uses rfind)
    std::string start_include;

    // Convenience headers (headefiles with solely purpose of including others
    std::string convenience_header_path;
    std::vector<std::string> convenience_headers;

    std::string skip_namespace;
    bool output_member_variables;

    enum output_style_type {def, alt};
    output_style_type output_style;

    configuration() 
      : output_style(def) 
      , output_member_variables(false) 
    {}
};


#endif // CONFIGURATION_HPP
