/*=============================================================================
    Copyright (c) 2001-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  This example shows:
//  1.  Parsing of different comment styles
//          parsing C/C++-style comment
//          parsing C++-style comment
//          parsing PASCAL-style comment
//  2.  Parsing tagged data with the help of the confix_parser
//  3.  Parsing tagged data with the help of the confix_parser but the semantic
//      action is directly attached to the body sequence parser
//
///////////////////////////////////////////////////////////////////////////////

#include <string>
#include <iostream>
#include <cassert>

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_confix.hpp>
#include <boost/spirit/include/classic_chset.hpp>


///////////////////////////////////////////////////////////////////////////////
// used namespaces
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
// actor called after successfully matching a single character
class actor_string
{
public:
    actor_string(std::string &rstr) :
        matched(rstr)
    {
    }

    void operator() (const char *pbegin, const char *pend) const
    {
        matched += std::string(pbegin, pend-pbegin);
    }

private:
    std::string &matched;
};

///////////////////////////////////////////////////////////////////////////////
// actor called after successfully matching a C++-comment
void actor_cpp (const char *pfirst, const char *plast)
{
    cout << "Parsing C++-comment" <<endl;
    cout << "Matched (" << plast-pfirst << ") characters: ";
    cout << "\"" << std::string(pfirst, plast) << "\"" << endl;
}

///////////////////////////////////////////////////////////////////////////////
// main entry point
int main ()
{

///////////////////////////////////////////////////////////////////////////////
//
//  1.  Parsing different comment styles
//      parsing C/C++-style comments (non-nested!)
//
///////////////////////////////////////////////////////////////////////////////

    char const* pCComment = "/* This is a /* nested */ C-comment */";

    rule<> cpp_comment;

    cpp_comment =
            comment_p("/*", "*/")           // rule for C-comments
        |   comment_p("//")                 // rule for C++ comments
        ;

    std::string comment_c;
    parse_info<> result;

    result = parse (pCComment, cpp_comment[actor_string(comment_c)]);
    if (result.hit)
    {
        cout << "Parsed C-comment successfully!" << endl;
        cout << "Matched (" << (int)comment_c.size() << ") characters: ";
        cout << "\"" << comment_c << "\"" << endl;
    }
    else
    {
        cout << "Failed to parse C/C++-comment!" << endl;
    }
    cout << endl;

    //        parsing C++-style comment
    char const* pCPPComment = "// This is a C++-comment\n";
    std::string comment_cpp;

    result = parse (pCPPComment, cpp_comment[&actor_cpp]);
    if (result.hit)
        cout << "Parsed C++-comment successfully!" << endl;
    else
        cout << "Failed to parse C++-comment!" << endl;

    cout << endl;


    //        parsing PASCAL-style comment (nested!)
    char const* pPComment = "{ This is a (* nested *) PASCAL-comment }";

    rule<> pascal_comment;

    pascal_comment =                    // in PASCAL we have two comment styles
            comment_nest_p('{', '}')    // both may be nested
        |   comment_nest_p("(*", "*)")
        ;

    std::string comment_pascal;

    result = parse (pPComment, pascal_comment[actor_string(comment_pascal)]);
    if (result.hit)
    {
        cout << "Parsed PASCAL-comment successfully!" << endl;
        cout << "Matched (" << (int)comment_pascal.size() << ") characters: ";
        cout << "\"" << comment_pascal << "\"" << endl;
    }
    else
    {
        cout << "Failed to parse PASCAL-comment!" << endl;
    }
    cout << endl;

///////////////////////////////////////////////////////////////////////////////
//
//  2.  Parsing tagged data with the help of the confix parser
//
///////////////////////////////////////////////////////////////////////////////

    std::string body;
    rule<> open_tag, html_tag, close_tag, body_text;

    open_tag =
            str_p("<b>")
        ;

    body_text =
            anychar_p
        ;

    close_tag =
            str_p("</b>")
        ;

    html_tag =
            confix_p (open_tag, (*body_text)[actor_string(body)], close_tag)
        ;

    char const* pTag = "<b>Body text</b>";

    result = parse (pTag, html_tag);
    if (result.hit)
    {
        cout << "Parsed HTML snippet \"<b>Body text</b>\" successfully "
            "(with re-attached actor)!" << endl;
        cout << "Found body (" << (int)body.size() << " characters): ";
        cout << "\"" << body << "\"" << endl;
    }
    else
    {
        cout << "Failed to parse HTML snippet (with re-attached actor)!"
            << endl;
    }
    cout << endl;

///////////////////////////////////////////////////////////////////////////////
//
//  3.  Parsing tagged data with the help of the confix_parser but the
//      semantic action is directly attached to the body sequence parser
//      (see comment in confix.hpp) and out of the usage of the 'direct()'
//      construction function no automatic refactoring takes place.
//
//      As you can see, for successful parsing it is required to refactor the
//      confix parser by hand. To see, how it fails, you can try the following:
//
//          html_tag_direct =
//              confix_p.direct(
//                  str_p("<b>"),
//                  (*body_text)[actor_string(bodydirect)],
//                  str_p("</b>")
//              )
//              ;
//
//      Here the *body_text parser eats up all the input up to the end of the
//      input sequence.
//
///////////////////////////////////////////////////////////////////////////////

    rule<> html_tag_direct;
    std::string bodydirect;

    html_tag_direct =
            confix_p.direct(
                str_p("<b>"),
                (*(body_text - str_p("</b>")))[actor_string(bodydirect)],
                str_p("</b>")
            )
        ;

    char const* pTagDirect = "<b>Body text</b>";

    result = parse (pTagDirect, html_tag_direct);
    if (result.hit)
    {
        cout << "Parsed HTML snippet \"<b>Body text</b>\" successfully "
            "(with direct actor)!" << endl;
        cout << "Found body (" << (int)bodydirect.size() << " characters): ";
        cout << "\"" << bodydirect << "\"" << endl;
    }
    else
    {
        cout << "Failed to parse HTML snippet (with direct actor)!" << endl;
    }
    cout << endl;

    return 0;
}
