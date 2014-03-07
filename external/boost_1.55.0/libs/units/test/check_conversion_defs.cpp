// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iterator>
#include <string>
#include <utility>
#include <set>
#include <map>
#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/regex.hpp>

namespace filesystem = boost::filesystem;

//struct stop {
//    stop() { char c; std::cin >> c; }
//} stop_;

boost::regex whitespace("\\s*");
boost::regex blank_line("\\A(?://.*$|\\s)*");

boost::regex include_guard("#ifndef (\\w+)\n#define \\1\n");
boost::regex base_unit("(\\w*_base_unit)(?:;| :)");

std::pair<std::string, std::string> get_base_unit_and_include_guard(const filesystem::path& path) {
    filesystem::ifstream in(path);
    std::string contents(std::istreambuf_iterator<char>(in.rdbuf()), std::istreambuf_iterator<char>());
    in.close();
    boost::smatch include_guard_match;
    boost::regex_search(contents, include_guard_match, include_guard);
    boost::smatch base_unit_match;
    boost::regex_search(contents, base_unit_match, base_unit);
    std::cout << "creating map entry: " << base_unit_match[1].str() << " -> "<<  include_guard_match[1].str() << std::endl;
    return(std::make_pair(base_unit_match[1].str(), include_guard_match[1].str()));
}

int main() {
    std::cout << "In main" << std::endl;
    std::map<std::string, std::string> include_guards;
    for(filesystem::directory_iterator begin(filesystem::path("../../../boost/units/systems/base_units")), end; begin != end; ++begin) {
        if(begin->status().type() == filesystem::regular_file) {
            std::cout << "reading file: " << begin->path() << std::endl; 
            include_guards.insert(get_base_unit_and_include_guard(begin->path()));
        }
    }

    std::cout << "reading conversions file" << std::endl;
    filesystem::ifstream conversions_file(filesystem::path("../../../boost/units/systems/base_units/detail/conversions.hpp"));

    std::string line;
    int line_count = 0;

    boost::smatch match;

    std::set<std::string> conversion_guards;

    try {

boost::regex include_guard_regex("#if defined\\((\\w+)\\) && defined\\((\\w+)\\) &&\\\\");
std::cout << __LINE__ << std::endl;
boost::regex conversion_guard_regex("    !defined\\((\\w+)\\)");
std::cout << __LINE__ << std::endl;
boost::regex set_conversion_guard("    #define (\\w+)");
std::cout << __LINE__ << std::endl;
boost::regex include_conversion("    #include <boost/units/conversion.hpp>");
std::cout << __LINE__ << std::endl;
boost::regex include_absolute("    #include <boost/units/absolute.hpp>");
std::cout << __LINE__ << std::endl;
boost::regex define_conversion_factor("    BOOST_UNITS_DEFINE_CONVERSION_FACTOR\\(boost::units::(\\w+_base_unit), boost::units::(\\w+_base_unit), \\w+, (?:[\\d\\.e\\-/ ]*|one\\(\\))\\);");
std::cout << __LINE__ << std::endl;
boost::regex define_conversion_offset("    BOOST_UNITS_DEFINE_CONVERSION_OFFSET\\(boost::units::(\\w+_base_unit), boost::units::(\\w+_base_unit), \\w+, [\\d\\.e+\\* \\-/]*\\);");
std::cout << __LINE__ << std::endl;
boost::regex endif("#endif");
std::cout << __LINE__ << std::endl;

    while(std::getline(conversions_file, line)) {
        ++line_count;
        std::cout << "on line: " << line_count << std::endl;
        if(boost::regex_match(line, match, blank_line)) {
            continue;
        } else if(boost::regex_match(line, match, include_guard_regex)) {
            std::string guard1, guard2, unit1, unit2, conversion_guard;
            bool uses_absolute = false;

            guard1 = match[1].str();
            guard2 = match[2].str();
            if(!std::getline(conversions_file, line)) { std::cerr << "unexpected end of file." << std::endl; return(1); } ++line_count;
            if(!boost::regex_match(line, match, conversion_guard_regex)) { std::cerr << "error on line: " << line_count << std::endl; return(1); }

            conversion_guard = match[1].str();
            if(!conversion_guards.insert(conversion_guard).second){ std::cerr << "error on line: " << line_count << std::endl; return(1); }

            if(!std::getline(conversions_file, line)) { std::cerr << "unexpected end of file." << std::endl; return(1); } ++line_count;
            if(!boost::regex_match(line, match, set_conversion_guard)) { std::cerr << "error on line: " << line_count << std::endl; return(1); }
std::cout << __LINE__ << std::endl;

            if(match[1].str() != conversion_guard) { std::cerr << "error on line: " << line_count << std::endl; return(1); }

            if(!std::getline(conversions_file, line)) { std::cerr << "unexpected end of file." << std::endl; return(1); } ++line_count;
            if(!boost::regex_match(line, match, include_conversion)) { std::cerr << "error on line: " << line_count << std::endl; return(1); }
std::cout << __LINE__ << std::endl;

            if(!std::getline(conversions_file, line)) { std::cerr << "unexpected end of file." << std::endl; return(1); } ++line_count;
            if(boost::regex_match(line, match, include_absolute)) {
                uses_absolute = true;
                if(!std::getline(conversions_file, line)) { std::cerr << "unexpected end of file." << std::endl; return(1); } ++line_count;
            }
            
std::cout << __LINE__ << std::endl;
            if(!boost::regex_match(line, match, define_conversion_factor)) { std::cerr << "error on line: " << line_count << std::endl; return(1); }

std::cout << __LINE__ << ": " << line << std::endl;
            unit1 = match[1].str();
            unit2 = match[2].str();
            if(!((include_guards[unit1] == guard1 && include_guards[unit2] == guard2) ||
                (include_guards[unit1] == guard2 && include_guards[unit2] == guard1))) {
                    std::cerr << "guard1: " << guard1 << std::endl;
                    std::cerr << "guard2: " << guard2 << std::endl;
                    std::cerr << "unit1: " << unit1 << std::endl;
                    std::cerr << "unit2: " << unit2 << std::endl;
                    std::cerr << "include_guards[unit1]: " << include_guards[unit1] << std::endl;
                    std::cerr << "include_guards[unit2]: " << include_guards[unit2] << std::endl;
                    { std::cerr << "error on line: " << line_count << std::endl; return(1); }
            }
std::cout << __LINE__ << std::endl;
            
            if(!std::getline(conversions_file, line)) { std::cerr << "unexpected end of file." << std::endl; return(1); } ++line_count;
std::cout << __LINE__ << std::endl;
            if(boost::regex_match(line, match, define_conversion_offset)) {
                if(!uses_absolute) { std::cerr << "error on line: " << line_count << std::endl; return(1); }
std::cout << __LINE__ << std::endl;
                if(match[1].str() != unit1 || match[2].str() != unit2) { std::cerr << "error on line: " << line_count << std::endl; return(1); }
                if(!std::getline(conversions_file, line)) { std::cerr << "unexpected end of file." << std::endl; return(1); } ++line_count;
            } else {
                if(uses_absolute) { std::cerr << "error on line: " << line_count << std::endl; return(1); }
            }
std::cout << __LINE__ << std::endl;


            if(!boost::regex_match(line, match, endif)) { std::cerr << "error on line: " << line_count << std::endl; return(1); }

        }
    }

    } catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return(1);
    }

}
