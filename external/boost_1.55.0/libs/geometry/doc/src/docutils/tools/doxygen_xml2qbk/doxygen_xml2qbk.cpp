// doxml2qbk (developed in the context of Boost.Geometry documentation)
//
// Copyright (c) 2010-2013 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2012-2013 Adam Wulkiewicz, Lodz, Poland.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//
// Barend Gehrels, Aug 1, 2010
// In continuation of the QuickBook documentation of Boost.Geometry
//
// Converts XML files created by Doxygen to Quickbook
// Notes:
// - basically generic, but implemented with Boost.Geometry in mind
// - makes use of some specific XML elements, which can be created by Doxygen
//       using /xmlonly
//     currently this is the element <qbk.example> which will make a reference
//     to an example.
// - currently still in draft

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>


#include <boost/program_options.hpp>

#include <rapidxml.hpp>

#include <configuration.hpp>
#include <file_to_string.hpp>
#include <doxygen_elements.hpp>
#include <doxygen_xml_parser.hpp>
#include <parameter_predicates.hpp>
#include <quickbook_output.hpp>
#include <rapidxml_util.hpp>

static const std::string version = "1.1.1";

inline std::string program_description(bool decorated)
{
    std::string result;
    if (decorated)
    {
        result = "=== ";
    }
    result += "doxygen_xml2qbk ";
    result += version;
    if (decorated)
    {
        result += " ===";
    }
    return result;
}


int main(int argc, char** argv)
{
    std::string filename;
    try
    {
        configuration config;
        std::string copyright_filename;
        std::string output_style;

        // Read/get configuration
        {
            namespace po = boost::program_options;
            po::options_description description;

            std::string convenience_headers;

            description.add_options()
                ("help", "Help message")
                ("version", "Version description")
                ("xml", po::value<std::string>(&filename), 
                            "Name of XML file written by Doxygen")
                ("start_include", po::value<std::string>(&config.start_include), 
                            "Start include")
                ("convenience_header_path", po::value<std::string>(&config.convenience_header_path), 
                            "Convenience header path")
                ("convenience_headers", po::value<std::string>(&convenience_headers), 
                            "Convenience header(s) (comma-separated)")
                ("skip_namespace", po::value<std::string>(&config.skip_namespace), 
                            "Namespace to skip (e.g. boost::mylib::)")
                ("copyright", po::value<std::string>(&copyright_filename), 
                            "Name of QBK file including (commented) copyright and license")

                ("output_style", po::value<std::string>(&output_style),
                            "Docbook output style. Available values: 'alt'")
                ("output_member_variables", po::value<bool>(&config.output_member_variables), 
                            "Output member variables inside the class")
            ;

            po::variables_map varmap;

            if (argc == 2 && ! boost::starts_with(argv[1], "--"))
            {
                // (especially for debugging) options might go into an INI file
                std::ifstream config_file (argv[1], std::ifstream::in);
                po::store(po::parse_config_file(config_file, description), varmap);
            }
            else
            {
                po::store(po::parse_command_line(argc, argv, description), varmap);
            }

            po::notify(varmap);

            if (varmap.count("version"))
            {
                std::cout << version << std::endl;
                return 0;
            }
            else if (varmap.count("help"))
            {
                std::cout 
                    << program_description(true) << std::endl 
                    << "Available options:" << std::endl
                    << description << std::endl;
                return 0;
            }
            else if (filename.empty())
            {
                std::cout 
                    << program_description(true) << std::endl 
                    << "Allowed options:" << std::endl
                    << description << std::endl;
                return 1;
            }

            // Split CSV with headerfile names into configuration
            if (! convenience_headers.empty())
            {
                boost::split(config.convenience_headers, convenience_headers, boost::is_any_of(","));
            }
        }

        // Set output style
        if ( "alt" == output_style )
            config.output_style = configuration::alt;

        // Read files into strings
        std::string xml_string = file_to_string(filename);
        std::string license = copyright_filename.empty() 
            ? "" 
            : file_to_string(copyright_filename);

        // Parse the XML outputted by Doxygen
        xml_doc xml(xml_string.c_str());

        documentation doc;
        parse(xml.first_node(), config, doc);

        // Check for duplicate function names
        for (std::size_t i = 0; i < doc.functions.size(); i++)
        {
            function& f1 = doc.functions[i];
            for (std::size_t j = i + 1; j < doc.functions.size(); j++)
            {
                function& f2 = doc.functions[j];

                if (f1.name == f2.name)
                {
                    // It is not a unique function, so e.g. an overload,
                    // so a description must distinguish them.
                    // Difference is either the number of parameters, or a const / non-const version
                    // Use the "\qbk{distinguish,with strategy}" in the source code to distinguish
                    f1.unique = false;
                    f2.unique = false;
                }
            }
        }


        // Write copyright/license (keep inspect silent)
        if (! license.empty())
        {
            std::cout << license << std::endl;
        }

        // Write warning comment
        std::cout
            << "[/ Generated by " << program_description(false) << ", don't change, will be overwritten automatically]" << std::endl
            << "[/ Generated from " << filename << "]" << std::endl;

        if ( configuration::def == config.output_style )
        {
            // Write the rest: functions, defines, classes or structs
            BOOST_FOREACH(function const& f, doc.functions)
            {
                quickbook_output(f, config, std::cout);
            }
            BOOST_FOREACH(function const& f, doc.defines)
            {
                quickbook_output(f, config, std::cout);
            }
            BOOST_FOREACH(enumeration const& e, doc.enumerations)
            {
                quickbook_output(e, config, std::cout);
            }

            if (! doc.cos.name.empty())
            {
                std::sort(doc.cos.functions.begin(), doc.cos.functions.end(), sort_on_line<function>());
                quickbook_output(doc.cos, config, std::cout);
            }
        }
        else if ( configuration::alt == config.output_style )
        {
            if (! doc.cos.name.empty())
            {
                std::sort(doc.cos.functions.begin(), doc.cos.functions.end(), sort_on_line<function>());
                quickbook_output_alt(doc.cos, config, std::cout);
            }

            if ( !doc.group_id.empty() )
                quickbook_output_alt(doc, config, std::cout);
        }
    }
    catch(std::exception const& e)
    {
        std::cerr << "Exception in doxygen_xml2qbk: " << std::endl
            << "   Message: " << e.what() << std::endl
            << "   File: " << filename << std::endl
            << "   Type: " << typeid(e).name() << std::endl
            << std::endl;
        return 1;
    }
    catch(...)
    {
        std::cerr << "Unknown exception in doxygen_xml2qbk" 
            << std::endl;
        return 1;
    }
    return 0;
}

