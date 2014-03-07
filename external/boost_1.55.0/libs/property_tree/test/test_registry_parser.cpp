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

// Only test registry parser if we have windows platform
#ifdef BOOST_WINDOWS

#include <boost/property_tree/registry_parser.hpp>
#include <boost/property_tree/info_parser.hpp>

///////////////////////////////////////////////////////////////////////////////
// Test data

const char *data_1 = 
    "root\n"
    "{\n"
    "  subkey1 \"default value 1\"\n"
    "  subkey2 \"default value 2\"\n"
    "  \\\\values\n"
    "  {\n"
    "    REG_NONE \"\"\n"
    "    REG_BINARY \"de ad be ef\"\n"
    "    REG_DWORD 1234567890\n"
    "    REG_QWORD 12345678901234567890\n"
    "    REG_SZ \"some text\"\n"
    "    REG_EXPAND_SZ \"some other text\"\n"
    "  }\n"
    "  \\\\types\n"
    "  {\n"
    "    REG_NONE 0\n"
    "    REG_BINARY 3\n"
    "    REG_DWORD 4\n"
    "    REG_QWORD 11\n"
    "    REG_SZ 1\n"
    "    REG_EXPAND_SZ 2\n"
    "  }\n"
    "}\n";

template<class Ptree>
void test_registry_parser()
{

    using namespace boost::property_tree;
    typedef typename Ptree::key_type::value_type Ch;
    typedef std::basic_string<Ch> Str;

    // Delete test registry key
    RegDeleteKeyA(HKEY_CURRENT_USER, "boost ptree test");
    
    // Get test ptree
    Ptree pt;
    std::basic_stringstream<Ch> stream(detail::widen<Ch>(data_1));
    read_info(stream, pt);

    try
    {

        // Write to registry, read back and compare contents
        Ptree pt2;
        write_registry(HKEY_CURRENT_USER, detail::widen<Ch>("boost ptree test"), pt);
        read_registry(HKEY_CURRENT_USER, detail::widen<Ch>("boost ptree test"), pt2);
        BOOST_CHECK(pt == pt2);

        // Test binary translation
        Str s = pt2.template get<Str>(detail::widen<Ch>("root.\\values.REG_BINARY"));
        std::vector<BYTE> bin = registry_parser::translate(REG_BINARY, s);
        BOOST_REQUIRE(bin.size() == 4);
        BOOST_CHECK(*reinterpret_cast<DWORD *>(&bin.front()) == 0xEFBEADDE);
        Str s2 = registry_parser::translate<Ch>(REG_BINARY, bin);
        BOOST_CHECK(s == s2);

    }
    catch (std::exception &e)
    {
        BOOST_ERROR(e.what());
    }
    
    // Delete test registry key
    RegDeleteKeyA(HKEY_CURRENT_USER, "boost ptree test");

}

int test_main(int argc, char *argv[])
{
    using namespace boost::property_tree;
    test_registry_parser<ptree>();
    //test_registry_parser<iptree>();
#ifndef BOOST_NO_CWCHAR
    //test_registry_parser<wptree>();
    //test_registry_parser<wiptree>();
#endif
    return 0;
}

#else

int test_main(int argc, char *argv[])
{
    return 0;
}

#endif
