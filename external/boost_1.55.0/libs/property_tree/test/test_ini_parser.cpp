// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------

#include "test_utils.hpp"
#include <boost/property_tree/ini_parser.hpp>
#include <sstream>

///////////////////////////////////////////////////////////////////////////////
// Test data

// Correct data
const char *ok_data_1 = 
    "\n"
    "; Comment\n"
    "[Section1]\n"
    "\t   \t; Comment\n"
    "  Key1=Data1\n"
    "            \n"
    "   Key2   =   Data2\n"
    "Key 3     =      Data 3  \n"
    "Key4=Data4\n"
    "[Section2] ;Comment\n"
    "\t   \tKey1=Data4\n";

// Correct data
const char *ok_data_2 = 
    "[Section1]\n"
    "Key1=Data1";              // No eol

// Correct data
const char *ok_data_3 = 
    "";

// Correct data
const char *ok_data_4 = 
    ";Comment";

// Correct data
const char *ok_data_5 = 
    "Key1=Data1\n"             // No section
    "Key2=Data2\n";

// Treat # as comment.
const char *ok_data_6 =
    "# Comment\n"
    "[Section1]\n"
    "Key1=Data1\n";

// Erroneous data
const char *error_data_1 = 
    "[Section1]\n"
    "Key1\n"                   // No equals sign
    "Key2=Data2";

// Erroneous data
const char *error_data_2 = 
    "[Section1]\n"
    "Key1=Data1\n"
    "=Data2\n";                // No key

struct ReadFunc
{
    template<class Ptree>
    void operator()(const std::string &filename, Ptree &pt) const
    {
        boost::property_tree::read_ini(filename, pt);
    }
};

struct WriteFunc
{
    template<class Ptree>
    void operator()(const std::string &filename, const Ptree &pt) const
    {
        boost::property_tree::write_ini(filename, pt);
    }
};

void test_erroneous_write(const boost::property_tree::ptree &pt)
{
    using namespace boost::property_tree;
    std::stringstream stream;
    try
    {
        write_ini(stream, pt);
        BOOST_ERROR("No required exception thrown");
    }
    catch (ini_parser_error &e)
    {
        (void)e;
    }
    catch (...)
    {
        BOOST_ERROR("Wrong exception type thrown");
    }
}

template<class Ptree>
void test_ini_parser()
{
    
    using namespace boost::property_tree;

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_1, NULL, 
        "testok1.ini", NULL, "testok1out.ini", 8, 26, 37
    );
    
    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_2, NULL, 
        "testok2.ini", NULL, "testok2out.ini", 3, 5, 12
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_3, NULL, 
        "testok3.ini", NULL, "testok3out.ini", 1, 0, 0
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_4, NULL, 
        "testok4.ini", NULL, "testok4out.ini", 1, 0, 0
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_5, NULL, 
        "testok5.ini", NULL, "testok5out.ini", 3, 10, 8
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_6, NULL, 
        "testok6.ini", NULL, "testok6out.ini", 3, 5, 12
    );

    generic_parser_test_error<Ptree, ReadFunc, WriteFunc, ini_parser_error>
    (
        ReadFunc(), WriteFunc(), error_data_1, NULL,
        "testerr1.ini", NULL, "testerr1out.ini", 2
    );

    generic_parser_test_error<Ptree, ReadFunc, WriteFunc, ini_parser_error>
    (
        ReadFunc(), WriteFunc(), error_data_2, NULL,
        "testerr2.ini", NULL, "testerr2out.ini", 3
    );

}

int test_main(int argc, char *argv[])
{

    using namespace boost::property_tree;

    test_ini_parser<ptree>();
    test_ini_parser<iptree>();
#ifndef BOOST_NO_CWCHAR
    test_ini_parser<wptree>();
    test_ini_parser<wiptree>();
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Too rich property tree tests

    // Test too deep ptrees
    {
        ptree pt;
        pt.put_child("section.key.bogus", ptree());
        test_erroneous_write(pt);
    }

    // Test duplicate sections
    {
        ptree pt;
        pt.push_back(std::make_pair("section", ptree()));
        pt.push_back(std::make_pair("section", ptree()));
        test_erroneous_write(pt);
    }

    // Test duplicate keys
    {
        ptree pt;
        ptree &child = pt.put_child("section", ptree());
        child.push_back(std::make_pair("key", ptree()));
        child.push_back(std::make_pair("key", ptree()));
        test_erroneous_write(pt);
    }

    // Test mixed data and children.
    {
        ptree pt;
        ptree &child = pt.put_child("section", ptree("value"));
        child.push_back(std::make_pair("key", ptree()));
        child.push_back(std::make_pair("key", ptree()));
        test_erroneous_write(pt);
    }

    return 0;

}
