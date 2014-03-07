//  Copyright (c) 2009 Chris Hoeppler
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The purpose of this example is to demonstrate different use cases for the
//  confix directive.

#include <iostream>
#include <string>
#include <vector>

//[qi_confix_includes
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/repository/include/qi_confix.hpp>
//]

namespace client {
//[qi_confix_using
    using boost::spirit::eol;
    using boost::spirit::lexeme;
    using boost::spirit::ascii::alnum;
    using boost::spirit::ascii::char_;
    using boost::spirit::ascii::space;
    using boost::spirit::qi::parse;
    using boost::spirit::qi::phrase_parse;
    using boost::spirit::repository::confix;
//]

//[qi_confix_cpp_comment
    template <typename Iterator>
    bool parse_cpp_comment(Iterator first, Iterator last, std::string& attr)
    {
        bool r = parse(first, last,
            confix("//", eol)[*(char_ - eol)],  // grammar
            attr);                              // attribute
        
        if (!r || first != last) // fail if we did not get a full match
            return false;
        return r;
    }
//]

//[qi_confix_c_comment
    template <typename Iterator>
    bool parse_c_comment(Iterator first, Iterator last, std::string& attr)
    {
        bool r = parse(first, last,
            confix("/*", "*/")[*(char_ - "*/")],    // grammar
            attr);                                  // attribute
        
        if (!r || first != last) // fail if we did not get a full match
            return false;
        return r;
    }
//]

//[qi_confix_tagged_data
    template <typename Iterator>
    bool parse_tagged(Iterator first, Iterator last, std::string& attr)
    {
        bool r = phrase_parse(first, last,
            confix("<b>", "</b>")[lexeme[*(char_ - '<')]],  // grammar
            space,                                          // skip
            attr);                                          // attribute
        
        if (!r || first != last) // fail if we did not get a full match
            return false;
        return r;
    }
//]
}


int main()
{
    // C++ comment
    std::string comment("// This is a comment\n");
    std::string attr;
    bool r = client::parse_cpp_comment(comment.begin(), comment.end(), attr);

    std::cout << "Parsing a C++ comment";
    if (r && attr == " This is a comment")
        std::cout << " succeeded." << std::endl;
    else
        std::cout << " failed" << std::endl;

    // C comment
    comment = "/* This is another comment */";
    attr.clear();
    r = client::parse_c_comment(comment.begin(), comment.end(), attr);

    std::cout << "Parsing a C comment";
    if (r && attr == " This is another comment ")
        std::cout << " succeeded." << std::endl;
    else
        std::cout << " failed" << std::endl;

    // Tagged data
    std::string data = "<b> This is the body. </b>";
    attr.clear();

    r = client::parse_tagged(data.begin(), data.end(), attr);

    std::cout << "Parsing tagged data";
    if (r && attr == "This is the body. ")
        std::cout << " succeeded." << std::endl;
    else
        std::cout << " failed" << std::endl;

    return 0;
}

