// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
// Copyright (C) 2009-2010 Sebastian Redl
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------

#include "test_xml_parser_common.hpp"
#include <locale>
#define BOOST_UTF8_BEGIN_NAMESPACE namespace boost { namespace property_tree {
#define BOOST_UTF8_END_NAMESPACE }}
#define BOOST_UTF8_DECL
#include <boost/detail/utf8_codecvt_facet.hpp>
#include <boost/detail/utf8_codecvt_facet.ipp>

int test_main(int argc, char *argv[])
{
    using namespace boost::property_tree;
    test_xml_parser<ptree>();
    test_xml_parser<iptree>();
#ifndef BOOST_NO_CWCHAR
    using std::locale;
    // We need a UTF-8-aware global locale now.
    locale loc(locale(), new utf8_codecvt_facet);
    locale::global(loc);
    test_xml_parser<wptree>();
    test_xml_parser<wiptree>();
#endif
    return 0;
}
