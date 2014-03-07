/*
 *
 * Copyright (c) 2002
 * John Maddock
 *
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include <iostream>
#include <fstream>
#include <iterator>
#include <cassert>
#include <boost/test/execution_monitor.hpp>
#include "./regex_comparison.hpp"

void test_match(const std::string& re, const std::string& text, const std::string& description)
{
    double time;
    results r(re, description);

    std::cout << "Testing: \"" << re << "\" against \"" << description << "\"" << std::endl;
    if(time_greta == true)
    {
        //      time = g::time_match(re, text);
        //      r.greta_time = time;
        //      std::cout << "\tGRETA regex: " << time << "s\n";
    }
    if(time_safe_greta == true)
    {
        //      time = gs::time_match(re, text);
        //      r.safe_greta_time = time;
        //      std::cout << "\tSafe GRETA regex: " << time << "s\n";
    }
    if(time_dynamic_xpressive == true)
    {
        time = dxpr::time_match(re, text);
        r.dynamic_xpressive_time = time;
        std::cout << "\tdynamic xpressive regex: " << time << "s\n";
    }
    if(time_static_xpressive == true)
    {
        time = sxpr::time_match(re, text);
        r.static_xpressive_time = time;
        std::cout << "\tstatic xpressive regex: " << time << "s\n";
    }
    if(time_boost == true)
    {
        time = b::time_match(re, text);
        r.boost_time = time;
        std::cout << "\tBoost regex: " << time << "s\n";
    }
    //if(time_posix == true)
    //{
    //   time = posix::time_match(re, text);
    //   r.posix_time = time;
    //   std::cout << "\tPOSIX regex: " << time << "s\n";
    //}
    //if(time_pcre == true)
    //{
    //   time = pcr::time_match(re, text);
    //   r.pcre_time = time;
    //   std::cout << "\tPCRE regex: " << time << "s\n";
    //}
    r.finalise();
    result_list.push_back(r);
}

void test_find_all(const std::string& re, const std::string& text, const std::string& description)
{
    std::cout << "Testing: " << re << std::endl;

    double time;
    results r(re, description);

#if defined(_MSC_VER) && (_MSC_VER >= 1300)
    if(time_greta == true)
    {
        //      time = g::time_find_all(re, text);
        //      r.greta_time = time;
        //      std::cout << "\tGRETA regex: " << time << "s\n";
    }
    if(time_safe_greta == true)
    {
        //      time = gs::time_find_all(re, text);
        //      r.safe_greta_time = time;
        //      std::cout << "\tSafe GRETA regex: " << time << "s\n";
    }
#endif
    if(time_dynamic_xpressive == true)
    {
        time = dxpr::time_find_all(re, text);
        r.dynamic_xpressive_time = time;
        std::cout << "\tdynamic xpressive regex: " << time << "s\n";
    }
    if(time_static_xpressive == true)
    {
        time = sxpr::time_find_all(re, text);
        r.static_xpressive_time = time;
        std::cout << "\tstatic xpressive regex: " << time << "s\n";
    }
    if(time_boost == true)
    {
        time = b::time_find_all(re, text);
        r.boost_time = time;
        std::cout << "\tBoost regex: " << time << "s\n";
    }
    //if(time_posix == true)
    //{
    //   time = posix::time_find_all(re, text);
    //   r.posix_time = time;
    //   std::cout << "\tPOSIX regex: " << time << "s\n";
    //}
    //if(time_pcre == true)
    //{
    //   time = pcr::time_find_all(re, text);
    //   r.pcre_time = time;
    //   std::cout << "\tPCRE regex: " << time << "s\n";
    //}
    r.finalise();
    result_list.push_back(r);
}

//int cpp_main(int argc, char**const argv)
int main(int argc, char**const argv)
{
    // start by processing the command line args:
    if(argc < 2)
        return show_usage();
    int result = 0;
    for(int c = 1; c < argc; ++c)
    {
        result += handle_argument(argv[c]);
    }
    if(result)
        return result;

    if(test_matches)
    {
        // these are from the regex docs:
        test_match("^([0-9]+)(\\-| |$)(.*)$", "100- this is a line of ftp response which contains a message string");
        test_match("([[:digit:]]{4}[- ]){3}[[:digit:]]{3,4}", "1234-5678-1234-456");
        // these are from http://www.regxlib.com/
        test_match("^([a-zA-Z0-9_\\-\\.]+)@((\\[[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.)|(([a-zA-Z0-9\\-]+\\.)+))([a-zA-Z]{2,4}|[0-9]{1,3})(\\]?)$", "john_maddock@compuserve.com");
        test_match("^([a-zA-Z0-9_\\-\\.]+)@((\\[[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.)|(([a-zA-Z0-9\\-]+\\.)+))([a-zA-Z]{2,4}|[0-9]{1,3})(\\]?)$", "foo12@foo.edu");
        test_match("^([a-zA-Z0-9_\\-\\.]+)@((\\[[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.)|(([a-zA-Z0-9\\-]+\\.)+))([a-zA-Z]{2,4}|[0-9]{1,3})(\\]?)$", "bob.smith@foo.tv");
        test_match("^[a-zA-Z]{1,2}[0-9][0-9A-Za-z]{0,1} {0,1}[0-9][A-Za-z]{2}$", "EH10 2QQ");
        test_match("^[a-zA-Z]{1,2}[0-9][0-9A-Za-z]{0,1} {0,1}[0-9][A-Za-z]{2}$", "G1 1AA");
        test_match("^[a-zA-Z]{1,2}[0-9][0-9A-Za-z]{0,1} {0,1}[0-9][A-Za-z]{2}$", "SW1 1ZZ");
        test_match("^[[:digit:]]{1,2}/[[:digit:]]{1,2}/[[:digit:]]{4}$", "4/1/2001");
        test_match("^[[:digit:]]{1,2}/[[:digit:]]{1,2}/[[:digit:]]{4}$", "12/12/2001");
        test_match("^[-+]?[[:digit:]]*\\.?[[:digit:]]*$", "123");
        test_match("^[-+]?[[:digit:]]*\\.?[[:digit:]]*$", "+3.14159");
        test_match("^[-+]?[[:digit:]]*\\.?[[:digit:]]*$", "-3.14159");

        output_xml_results(true, "Short Matches", "short_matches.xml");
    }
    std::string twain;

    if(test_short_twain)
    {
        load_file(twain, "short_twain.txt");

        test_find_all("Twain", twain);
        test_find_all("Huck[[:alpha:]]+", twain);
        test_find_all("[[:alpha:]]+ing", twain);
        test_find_all("^[^\n]*?Twain", twain);
        test_find_all("Tom|Sawyer|Huckleberry|Finn", twain);
        test_find_all("(Tom|Sawyer|Huckleberry|Finn).{0,30}river|river.{0,30}(Tom|Sawyer|Huckleberry|Finn)", twain);

        output_xml_results(false, "Moderate Searches", "short_twain_search.xml");
    }

    if(test_long_twain)
    {
        load_file(twain, "3200.txt");

        test_find_all("Twain", twain);
        test_find_all("Huck[[:alpha:]]+", twain);
        test_find_all("[[:alpha:]]+ing", twain);
        test_find_all("^[^\n]*?Twain", twain);
        test_find_all("Tom|Sawyer|Huckleberry|Finn", twain);
        //time_posix = false;
        test_find_all("(Tom|Sawyer|Huckleberry|Finn).{0,30}river|river.{0,30}(Tom|Sawyer|Huckleberry|Finn)", twain);
        //time_posix = true;

        output_xml_results(false, "Long Searches", "long_twain_search.xml");
    }   

    return 0;
}
