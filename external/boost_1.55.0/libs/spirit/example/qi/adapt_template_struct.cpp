//  Copyright (c) 2001-2010 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This example demonstrates a trick allowing to adapt a template data 
//  structure as a Fusion sequence in order to use is for direct attribute
//  propagation. For more information see 
//  http://boost-spirit.com/home/2010/02/08/how-to-adapt-templates-as-a-fusion-sequence

#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace fusion = boost::fusion;

namespace client
{
    template <typename A, typename B>
    struct data
    {
        A a;
        B b;
    };

    template <typename Iterator, typename A, typename B>
    struct data_grammar : qi::grammar<Iterator, data<A, B>()>
    {
        data_grammar() : data_grammar::base_type(start) 
        {
            start = real_start;
            real_start = qi::auto_ >> ',' >> qi::auto_;
        }

        qi::rule<Iterator, data<A, B>()> start;
        qi::rule<Iterator, fusion::vector<A&, B&>()> real_start;
    };
}

namespace boost { namespace spirit { namespace traits 
{
    template <typename A, typename B>
    struct transform_attribute<client::data<A, B>, fusion::vector<A&, B&>, qi::domain>
    {
        typedef fusion::vector<A&, B&> type;

        static type pre(client::data<A, B>& val) { return type(val.a, val.b); }
        static void post(client::data<A, B>&, fusion::vector<A&, B&> const&) {}
        static void fail(client::data<A, B>&) {}
    };
}}}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "\t\tA parser for Spirit utilizing an adapted template ...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";

    std::cout << "Give me two comma separated integers:\n";
    std::cout << "Type [q or Q] to quit\n\n";

    std::string str;
    client::data_grammar<std::string::const_iterator, long, int> g; // Our grammar
    while (getline(std::cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        client::data<long, int> d;
        std::string::const_iterator iter = str.begin();
        std::string::const_iterator end = str.end();
        bool r = phrase_parse(iter, end, g, qi::space, d);

        if (r && iter == end)
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded\n";
            std::cout << "got: " << d.a << "," << d.b << std::endl;
            std::cout << "\n-------------------------\n";
        }
        else
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed\n";
            std::cout << "-------------------------\n";
        }
    }

    return 0;
}


