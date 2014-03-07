//  (C) Copyright Gennadiy Rozental 2005-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 49312 $
//
//  Description : XML report formatter
// ***************************************************************************

#ifndef BOOST_TEST_XML_REPORT_FORMATTER_IPP_020105GER
#define BOOST_TEST_XML_REPORT_FORMATTER_IPP_020105GER

// Boost.Test
#include <boost/test/results_collector.hpp>
#include <boost/test/unit_test_suite_impl.hpp>
#include <boost/test/output/xml_report_formatter.hpp>

#include <boost/test/utils/xml_printer.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {

namespace unit_test {

namespace output {

void
xml_report_formatter::results_report_start( std::ostream& ostr )
{
    ostr << "<TestResult>";
}

//____________________________________________________________________________//

void
xml_report_formatter::results_report_finish( std::ostream& ostr )
{
    ostr << "</TestResult>";
}


//____________________________________________________________________________//

void
xml_report_formatter::test_unit_report_start( test_unit const& tu, std::ostream& ostr )
{
    test_results const& tr = results_collector.results( tu.p_id );

    const_string descr;

    if( tr.passed() )
        descr = "passed";
    else if( tr.p_skipped )
        descr = "skipped";
    else if( tr.p_aborted )
        descr = "aborted";
    else
        descr = "failed";

    ostr << '<' << ( tu.p_type == tut_case ? "TestCase" : "TestSuite" ) 
         << " name"     << attr_value() << tu.p_name.get()
         << " result"   << attr_value() << descr
         << " assertions_passed"        << attr_value() << tr.p_assertions_passed
         << " assertions_failed"        << attr_value() << tr.p_assertions_failed
         << " expected_failures"        << attr_value() << tr.p_expected_failures;

    if( tu.p_type == tut_suite )
        ostr << " test_cases_passed"    << attr_value() << tr.p_test_cases_passed
             << " test_cases_failed"    << attr_value() << tr.p_test_cases_failed
             << " test_cases_skipped"   << attr_value() << tr.p_test_cases_skipped
             << " test_cases_aborted"   << attr_value() << tr.p_test_cases_aborted;
             
    
    ostr << '>';
}

//____________________________________________________________________________//

void
xml_report_formatter::test_unit_report_finish( test_unit const& tu, std::ostream& ostr )
{
    ostr << "</" << ( tu.p_type == tut_case ? "TestCase" : "TestSuite" ) << '>';
}

//____________________________________________________________________________//

void
xml_report_formatter::do_confirmation_report( test_unit const& tu, std::ostream& ostr )
{
    test_unit_report_start( tu, ostr );
    test_unit_report_finish( tu, ostr );    
}

//____________________________________________________________________________//

} // namespace output

} // namespace unit_test

} // namespace boost

//____________________________________________________________________________//

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_XML_REPORT_FORMATTER_IPP_020105GER
