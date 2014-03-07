//  Copyright (c) 2011 Aaron Graham
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The purpose of this example is to demonstrate the use of the advance parser.

//[qi_advance_includes
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/repository/include/qi_advance.hpp>
//]

#include <list>
#include <string>

//[qi_advance_namespaces
namespace qi = boost::spirit::qi;
using boost::spirit::repository::qi::advance;
//]

namespace client
{
    //[qi_advance_grammar
    template <typename Iterator>
    struct advance_grammar : qi::grammar<Iterator, qi::locals<int> >
    {
        advance_grammar() : advance_grammar::base_type(start)
        {
            using qi::byte_;
            using qi::eoi;
            using namespace qi::labels;

            start
                =  byte_  [_a = _1]
                >> advance(_a)
                >> "boost"
                >> byte_  [_a = _1]
                >> (advance(_a) | "qi") // note alternative when advance fails
                >> eoi
                ;
        }

        qi::rule<Iterator, qi::locals<int> > start;
    };
    //]
}

int main()
{
    // This parser is tested with both random access iterators (std::string)
    // and bidirectional iterators (std::list).
    char const* result;

    //[qi_advance_example1
    unsigned char const alt1[] =
    {
        5,                         // number of bytes to advance
        1, 2, 3, 4, 5,             // data to advance through
        'b', 'o', 'o', 's', 't',   // word to parse
        2,                         // number of bytes to advance
        11, 12                     // more data to advance through
        // eoi
    };

    std::string const alt1_string(alt1, alt1 + sizeof alt1);
    std::list<unsigned char> const alt1_list(alt1, alt1 + sizeof alt1);

    result =
        qi::parse(alt1_string.begin(), alt1_string.end()
            , client::advance_grammar<std::string::const_iterator>())
        ? "succeeded" : "failed";
    std::cout << "Parsing alt1 using random access iterator " << result << std::endl;

    result =
        qi::parse(alt1_list.begin(), alt1_list.end()
            , client::advance_grammar<std::list<unsigned char>::const_iterator>())
        ? "succeeded" : "failed";
    std::cout << "Parsing alt1 using bidirectional iterator " << result << std::endl;
    //]

    //[qi_advance_example2
    unsigned char const alt2[] =
    {
        2,                         // number of bytes to advance
        1, 2,                      // data to advance through
        'b', 'o', 'o', 's', 't',   // word to parse
        4,                         // number of bytes to advance
        'q', 'i'                   // alternative (advance won't work)
        // eoi
    };

    std::string const alt2_string(alt2, alt2 + sizeof alt2);
    std::list<unsigned char> const alt2_list(alt2, alt2 + sizeof alt2);

    result =
        qi::parse(alt2_string.begin(), alt2_string.end()
            , client::advance_grammar<std::string::const_iterator>())
        ? "succeeded" : "failed";
    std::cout << "Parsing alt2 using random access iterator " << result << std::endl;

    result =
        qi::parse(alt2_list.begin(), alt2_list.end()
            , client::advance_grammar<std::list<unsigned char>::const_iterator>())
        ? "succeeded" : "failed";
    std::cout << "Parsing alt2 using bidirectional iterator " << result << std::endl;
    //]

    return 0;
}
