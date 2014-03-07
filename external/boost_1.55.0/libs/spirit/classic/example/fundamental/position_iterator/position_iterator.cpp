/*=============================================================================
    Copyright (c) 2002 Juan Carlos Arevalo-Baeza
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  A parser for a comma separated list of numbers,
//  with positional error reporting
//  See the "Position Iterator" chapter in the User's Guide.
//
//  [ JCAB 9/28/2002 ]
//
///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <boost/spirit/include/classic_functor_parser.hpp>
#include <iostream>
#include <fstream>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
//
//  Our error reporting parsers
//
///////////////////////////////////////////////////////////////////////////////
std::ostream& operator<<(std::ostream& out, file_position const& lc)
{
    return out <<
            "\nFile:\t" << lc.file <<
            "\nLine:\t" << lc.line <<
            "\nCol:\t" << lc.column << endl;
}

struct error_report_parser {
    char const* eol_msg;
    char const* msg;

    error_report_parser(char const* eol_msg_, char const* msg_):
        eol_msg(eol_msg_),
        msg    (msg_)
    {}

    typedef nil_t result_t;

    template <typename ScannerT>
    int
    operator()(ScannerT const& scan, result_t& /*result*/) const
    {
        if (scan.at_end()) {
            if (eol_msg) {
                file_position fpos = scan.first.get_position();
                cerr << fpos << eol_msg << endl;
            }
        } else {
            if (msg) {
                file_position fpos = scan.first.get_position();
                cerr << fpos << msg << endl;
            }
        }

        return -1; // Fail.
    }

};
typedef functor_parser<error_report_parser> error_report_p;

error_report_p
error_badnumber_or_eol =
    error_report_parser(
        "Expecting a number, but found the end of the file\n",
        "Expecting a number, but found something else\n"
    );

error_report_p
error_badnumber =
    error_report_parser(
        0,
        "Expecting a number, but found something else\n"
    );

error_report_p
error_comma =
    error_report_parser(
        0,
        "Expecting a comma, but found something else\n"
    );

///////////////////////////////////////////////////////////////////////////////
//
//  Our comma separated list parser
//
///////////////////////////////////////////////////////////////////////////////
bool
parse_numbers(char const* filename, char const* str, vector<double>& v)
{
    typedef position_iterator<char const*> iterator_t;
    iterator_t begin(str, str + strlen(str), filename);
    iterator_t end;
    begin.set_tabchars(8);
    return parse(begin, end,

        //  Begin grammar
        (
            (real_p[push_back_a(v)] | error_badnumber)
         >> *(
                (',' | error_comma)
             >> (real_p[push_back_a(v)] | error_badnumber_or_eol)
            )
        )
        ,
        //  End grammar

        space_p).full;
}

////////////////////////////////////////////////////////////////////////////
//
//  Main program
//
////////////////////////////////////////////////////////////////////////////
int
main(int argc, char **argv)
{
    cout << "/////////////////////////////////////////////////////////\n\n";
    cout << "\tAn error-reporting parser for Spirit...\n\n";
    cout << "Parses a comma separated list of numbers from a file.\n";
    cout << "The numbers will be inserted in a vector of numbers\n\n";
    cout << "/////////////////////////////////////////////////////////\n\n";

    char str[65536];
    char const* filename;

    if (argc > 1) {
        filename = argv[1];
        ifstream file(filename);
        file.get(str, sizeof(str), '\0');
    } else {
        filename = "<cin>";
        cin.get(str, sizeof(str), '\0');
    }

    vector<double> v;
    if (parse_numbers(filename, str, v))
    {
        cout << "-------------------------\n";
        cout << "Parsing succeeded\n";
        cout << str << " Parses OK: " << endl;

        for (vector<double>::size_type i = 0; i < v.size(); ++i)
            cout << i << ": " << v[i] << endl;

        cout << "-------------------------\n";
    }
    else
    {
        cout << "-------------------------\n";
        cout << "Parsing failed\n";
        cout << "-------------------------\n";
    }

    cout << "Bye... :-) \n\n";
    return 0;
}


