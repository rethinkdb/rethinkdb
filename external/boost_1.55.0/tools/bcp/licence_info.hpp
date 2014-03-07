/*
 *
 * Copyright (c) 2003 Dr John Maddock
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include <boost/regex.hpp>
#include <utility>

struct license_info
{
   boost::regex license_signature;
   boost::regex copyright_signature;
   std::string  copyright_formatter;
   std::string  license_name;
   std::string  license_text;
   //
   // we should really be able to initialize license_info as an
   // aggregate, but some compilers reject this, so use a constructor
   //instead:
   //
   license_info(const boost::regex& e1, 
            const boost::regex& e2, 
            const std::string& s1, 
            const std::string& s2, 
            const std::string& s3)
            : license_signature(e1), 
              copyright_signature(e2),
              copyright_formatter(s1),
              license_name(s2),
              license_text(s3){}
};

std::pair<const license_info*, int> get_licenses();

std::string format_authors_name(const std::string& name);
