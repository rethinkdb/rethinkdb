// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------
#ifndef TEST_XML_PARSER_COMMON_HPP_INCLUDED
#define TEST_XML_PARSER_COMMON_HPP_INCLUDED

#include "test_utils.hpp"
#include <boost/property_tree/xml_parser.hpp>
#include "xml_parser_test_data.hpp"

struct ReadFuncWS
{
    template<class Ptree>
    void operator()(const std::string &filename, Ptree &pt) const
    {
        boost::property_tree::read_xml(filename, pt,
          boost::property_tree::xml_parser::no_concat_text);
    }
};

struct WriteFuncWS
{
    template<class Ptree>
    void operator()(const std::string &filename, const Ptree &pt) const
    {
        boost::property_tree::write_xml(filename, pt);
    }
};

struct ReadFuncNS
{
    template<class Ptree>
    void operator()(const std::string &filename, Ptree &pt) const
    {
        boost::property_tree::read_xml(filename, pt,
          boost::property_tree::xml_parser::trim_whitespace);
    }
};

struct WriteFuncNS
{
    template<class Ptree>
    void operator()(const std::string &filename, const Ptree &pt) const
    {
        boost::property_tree::write_xml(filename, pt, std::locale(),
            boost::property_tree::xml_writer_make_settings(
                typename Ptree::key_type::value_type(' '), 4));
    }
};

template <typename Ch> int umlautsize();
template <> inline int umlautsize<char>() { return 2; }
template <> inline int umlautsize<wchar_t>() { return 1; }

template<class Ptree>
void test_xml_parser()
{

    using namespace boost::property_tree;
    typedef typename Ptree::data_type::value_type char_type;

    generic_parser_test_ok<Ptree, ReadFuncWS, WriteFuncWS>
    (
        ReadFuncWS(), WriteFuncWS(), ok_data_1, NULL, 
        "testok1.xml", NULL, "testok1out.xml", 2, 0, 5
    );

    generic_parser_test_ok<Ptree, ReadFuncWS, WriteFuncWS>
    (
        ReadFuncWS(), WriteFuncWS(), ok_data_2, NULL, 
        "testok2a.xml", NULL, "testok2aout.xml", 15, 23, 89
    );

    generic_parser_test_ok<Ptree, ReadFuncNS, WriteFuncNS>
    (
        ReadFuncNS(), WriteFuncNS(), ok_data_2, NULL, 
        "testok2b.xml", NULL, "testok2bout.xml", 6, 15, 8
    );

    generic_parser_test_ok<Ptree, ReadFuncWS, WriteFuncWS>
    (
        ReadFuncWS(), WriteFuncWS(), ok_data_3, NULL, 
        "testok3a.xml", NULL, "testok3aout.xml", 1662, 35377, 11706
    );

    generic_parser_test_ok<Ptree, ReadFuncNS, WriteFuncNS>
    (
        ReadFuncNS(), WriteFuncNS(), ok_data_3, NULL, 
        "testok3b.xml", NULL, "testok3bout.xml", 787, 31376, 3831
    );

    generic_parser_test_ok<Ptree, ReadFuncWS, WriteFuncWS>
    (
        ReadFuncWS(), WriteFuncWS(), ok_data_4, NULL, 
        "testok4.xml", NULL, "testok4out.xml", 11, 7, 74
    );

    generic_parser_test_ok<Ptree, ReadFuncWS, WriteFuncWS>
    (
        ReadFuncWS(), WriteFuncWS(), ok_data_5, NULL, 
        "testok5.xml", NULL, "testok5out.xml",
        3, umlautsize<char_type>(), 12
    );

    generic_parser_test_error<Ptree, ReadFuncWS, WriteFuncWS, xml_parser_error>
    (
        ReadFuncWS(), WriteFuncWS(), error_data_1, NULL,
        "testerr1.xml", NULL, "testerr1out.xml", 1
    );

    generic_parser_test_error<Ptree, ReadFuncWS, WriteFuncWS, xml_parser_error>
    (
        ReadFuncWS(), WriteFuncWS(), error_data_2, NULL,
        "testerr2.xml", NULL, "testerr2out.xml", 2
    );

    generic_parser_test_ok<Ptree, ReadFuncWS, WriteFuncWS>
    (
        ReadFuncWS(), WriteFuncWS(), bug_data_pr2855, NULL,
        "testpr2855.xml", NULL, "testpr2855out.xml", 3, 7, 14
    );
    
    generic_parser_test_ok<Ptree, ReadFuncWS, WriteFuncWS>
    (
        ReadFuncWS(), WriteFuncWS(), bug_data_pr1678, NULL,
        "testpr1678.xml", NULL, "testpr1678out.xml", 2, 0, 4
    );

    generic_parser_test_ok<Ptree, ReadFuncWS, WriteFuncWS>
    (
        ReadFuncWS(), WriteFuncWS(), bug_data_pr5203, NULL,
        "testpr5203.xml", NULL, "testpr5203out.xml",
        3, 4 * umlautsize<char_type>(), 13
    );

    generic_parser_test_ok<Ptree, ReadFuncWS, WriteFuncWS>
    (
        ReadFuncWS(), WriteFuncWS(), bug_data_pr4840, NULL,
        "testpr4840.xml", NULL, "testpr4840out.xml",
        4, 13, 15
    );

}

#endif
