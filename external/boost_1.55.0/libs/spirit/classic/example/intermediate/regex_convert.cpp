/*=============================================================================
    Copyright (c) 2002-2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
// vim:ts=4:sw=4:et
//
//  Demonstrate regular expression parsers for match based text conversion
//
//  This sample requires an installed version of the boost regex library
//  (http://www.boost.org) The sample was tested with boost V1.29.0
//
//  Note: - there is no error handling in this example
//        - this program isn't particularly useful
//
//  This example shows one way build a kind of filter program.
//  It reads input from std::cin and uses a grammar and actions
//  to print out a modified version of the input.
//
//  [ Martin Wille, 10/18/2002 ]
//
///////////////////////////////////////////////////////////////////////////////

#include <string>
#include <iostream>
#include <streambuf>
#include <sstream>
#include <deque>
#include <iterator>

#include <boost/function.hpp>
#include <boost/spirit/include/classic_core.hpp>

///////////////////////////////////////////////////////////////////////////////
//
//  The following header must be included, if regular expression support is
//  required for Spirit.
//
//  The BOOST_SPIRIT_NO_REGEX_LIB PP constant should be defined, if you're using the
//  Boost.Regex library from one translation unit only. Otherwise you have to
//  link with the Boost.Regex library as defined in the related documentation
//  (see. http://www.boost.org).
//
///////////////////////////////////////////////////////////////////////////////
#define BOOST_SPIRIT_NO_REGEX_LIB
#include <boost/spirit/include/classic_regex.hpp>

using namespace BOOST_SPIRIT_CLASSIC_NS;
using namespace std;

namespace {
    long triple(long val)
    {
        return 3*val;
    }
    ///////////////////////////////////////////////////////////////////////////
    //
    // actions
    //
    struct emit_constant
    {
        emit_constant(string const &text)
            : msg(text)
        {}

        template<typename Iterator>
        void operator()(Iterator b, Iterator e) const
        {
            cout.rdbuf()->sputn(msg.data(), msg.size());
        }

    private:

        string msg;
    };

    void
    copy_unmodified(char letter)
    {
        cout.rdbuf()->sputc(letter);
    }

    struct emit_modified_subscript
    {
        emit_modified_subscript(boost::function<long (long)> const &f)
            : modifier(f)
        {}

        template<typename Iterator>
        void operator()(Iterator b, Iterator e) const
        {
            string tmp(b+1,e-1);
            long val = strtol(tmp.c_str(),0, 0);
            ostringstream os;
            os << modifier(val);
            tmp = os.str();
            cout.rdbuf()->sputc('[');
            cout.rdbuf()->sputn(tmp.c_str(), tmp.size());
            cout.rdbuf()->sputc(']');
        }

    private:

        boost::function<long (long)> modifier;
    };
}

///////////////////////////////////////////////////////////////////////////////
//  The grammar 'conversion_grammar' serves as a working horse for match based
//  text conversion. It does the following:
//
//      - converts the word "class" into the word "struct"
//      - multiplies any integer number enclosed in square brackets with 3
//      - any other input is simply copied to the output

struct conversion_grammar
     : grammar<conversion_grammar>
{
    template<class ScannerT>
    struct definition
    {
        typedef ScannerT scanner_t;

        definition(conversion_grammar const &)
        {
            static const char expr[] = "\\[\\d+\\]";
            first = (
                /////////////////////////////////////////////////////////////
                // note that "fallback" is the last alternative here !
                top  = *(class2struct || subscript || fallback),
                /////////////////////////////////////////////////////////////
                // replace any occurrance of "class" by "struct"
                class2struct = str_p("class") [emit_constant("struct")],
                /////////////////////////////////////////////////////////////
                // if the input maches "[some_number]"
                // "some_number" is multiplied by 3 before printing
                subscript = regex_p(expr) [emit_modified_subscript(&triple)],
                /////////////////////////////////////////////////////////////
                // if nothing else can be done with the input
                // then it will be printed without modifications
                fallback = anychar_p [&copy_unmodified]
            );
        }

        rule<scanner_t> const & start() { return first; }

    private:

        subrule<0> top;
        subrule<1> class2struct;
        subrule<2> subscript;
        subrule<3> fallback;
        rule<scanner_t> first;
    };
};

int
main()
{
    //  this would print "struct foo {}; foo bar[9];":
    //  parse("class foo {}; foo bar[3];", conversion_grammar());

    // Note: the regular expression parser contained in the
    //       grammar requires a bidirectional iterator. Therefore,
    //       we cannot use sdt::istreambuf_iterator as one would
    //       do with other Spirit parsers.
    istreambuf_iterator<char> input_iterator(cin);
    std::deque<char> input(input_iterator, istreambuf_iterator<char>());

    parse(input.begin(), input.end(), conversion_grammar());
    return 0;
}

