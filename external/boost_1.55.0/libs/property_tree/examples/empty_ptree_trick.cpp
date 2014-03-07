// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <iostream>
#include <iomanip>
#include <string>

using namespace boost::property_tree;

// Process settings using empty ptree trick. Note that it is considerably simpler 
// than version which does not use the "trick"
void process_settings(const std::string &filename)
{
    ptree pt;
    read_info(filename, pt);    
    const ptree &settings = pt.get_child("settings", empty_ptree<ptree>());
    std::cout << "\n    Processing " << filename << std::endl;
    std::cout << "        Setting 1 is " << settings.get("setting1", 0) << std::endl;
    std::cout << "        Setting 2 is " << settings.get("setting2", 0.0) << std::endl;
    std::cout << "        Setting 3 is " << settings.get("setting3", "default") << std::endl;
}

// Process settings not using empty ptree trick. This one must duplicate much of the code.
void process_settings_without_trick(const std::string &filename)
{
    ptree pt;
    read_info(filename, pt);    
    if (boost::optional<ptree &> settings = pt.get_child_optional("settings"))
    {
        std::cout << "\n    Processing " << filename << std::endl;
        std::cout << "        Setting 1 is " << settings.get().get("setting1", 0) << std::endl;
        std::cout << "        Setting 2 is " << settings.get().get("setting2", 0.0) << std::endl;
        std::cout << "        Setting 3 is " << settings.get().get("setting3", "default") << std::endl;
    }
    else
    {
        std::cout << "\n    Processing " << filename << std::endl;
        std::cout << "        Setting 1 is " << 0 << std::endl;
        std::cout << "        Setting 2 is " << 0.0 << std::endl;
        std::cout << "        Setting 3 is " << "default" << std::endl;
    }
}

int main()
{
    try
    {
        std::cout << "Processing settings with empty-ptree-trick:\n";
        process_settings("settings_fully-existent.info");
        process_settings("settings_partially-existent.info");
        process_settings("settings_non-existent.info");
        std::cout << "\nProcessing settings without empty-ptree-trick:\n";
        process_settings_without_trick("settings_fully-existent.info");
        process_settings_without_trick("settings_partially-existent.info");
        process_settings_without_trick("settings_non-existent.info");
    }
    catch (std::exception &e)
    {
        std::cout << "Error: " << e.what() << "\n";
    }
    return 0;
}
