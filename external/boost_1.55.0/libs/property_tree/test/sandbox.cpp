// ----------------------------------------------------------------------------
// Copyright (C) 2009 Sebastian Redl
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------

#define _CRT_SECURE_NO_DEPRECATE
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <iostream>

int main()
{
    using namespace boost::property_tree;
    ptree pt;
    read_xml("simple_all.xml", pt);
    write_info(std::cout, pt);
}
