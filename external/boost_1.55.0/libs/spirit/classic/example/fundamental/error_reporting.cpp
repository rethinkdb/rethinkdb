/*=============================================================================
    Copyright (c) 2003 Pavel Baranov
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
/////////////////////////////////////////////////////////////////////////////// 
//
//  An alternate error-handling scheme where the parser will  
//  complain (but not stop) if input doesn't match.
//  
//  [ Pavel Baranov 8/27/2003 ]
// 
/////////////////////////////////////////////////////////////////////////////// 
#include <boost/spirit/include/classic_core.hpp> 
#include <boost/spirit/include/classic_functor_parser.hpp> 
#include <iostream> 
#include <string> 

/////////////////////////////////////////////////////////////////////////////// 
using namespace std; 
using namespace BOOST_SPIRIT_CLASSIC_NS; 

static short errcount = 0; 

/////////////////////////////////////////////////////////////////////////////// 
// 
//  Error reporting parser 
// 
/////////////////////////////////////////////////////////////////////////////// 
struct error_report_parser { 
    
    error_report_parser(const char *msg) : _msg(msg) {}

    typedef nil_t result_t; 
    
    template <typename ScannerT> 
    int operator()(ScannerT const& scan, result_t& /*result*/) const 
    { 
        errcount++; 
        cerr << _msg << endl; 
        return 0; 
    } 

private: 
    string _msg; 
}; 

typedef functor_parser<error_report_parser> error_report_p; 

/////////////////////////////////////////////////////////////////////////////// 
// 
//  My grammar 
// 
/////////////////////////////////////////////////////////////////////////////// 
struct my_grammar : public grammar<my_grammar> 
{ 
    static error_report_p error_missing_semicolon; 
    static error_report_p error_missing_letter; 

    template <typename ScannerT> 
    struct definition 
    { 
        definition(my_grammar const& self) : 
            SEMICOLON(';') 
        { 
            my_rule 
                = *(eps_p(alpha_p|SEMICOLON) >> 
                   (alpha_p|error_missing_letter) >> 
                   (SEMICOLON|error_missing_semicolon)) 
        ; 
        } 

        chlit<> 
            SEMICOLON; 

        rule<ScannerT> my_rule; 

        rule<ScannerT> const& 
        start() const { return my_rule; } 
    }; 
}; 

error_report_p my_grammar::error_missing_semicolon("missing semicolon"); 
error_report_p my_grammar::error_missing_letter("missing letter"); 

/////////////////////////////////////////////////////////////////////////////// 
// 
//  Main program 
// 
/////////////////////////////////////////////////////////////////////////////// 
int 
main() 
{ 
    cout << "/////////////////////////////////////////////////////////\n\n"; 
    cout << " Error handling demo\n\n"; 
    cout << " The parser expects a sequence of letter/semicolon pairs\n"; 
    cout << " and will complain (but not stop) if input doesn't match.\n\n"; 
    cout << "/////////////////////////////////////////////////////////\n\n"; 

    my_grammar g; 

    string str( "a;;b;cd;e;fg;" ); 
    cout << "input: " << str << "\n\n"; 

    if( parse(str.c_str(), g, space_p).full && !errcount ) 
        cout << "\nparsing succeeded\n"; 
    else 
        cout << "\nparsing failed\n"; 

    return 0; 
}
