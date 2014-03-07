/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2009 Francois Barel

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  A mini XML-like parser
//
//  [ JDG March 25, 2007 ]   spirit2
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/config/warning_disable.hpp>
//[mini_xml2_sr_includes
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/repository/include/qi_subrule.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
//]
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/foreach.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace client
{
    namespace fusion = boost::fusion;
    namespace phoenix = boost::phoenix;
    //[mini_xml2_sr_using
    namespace qi = boost::spirit::qi;
    namespace repo = boost::spirit::repository;
    namespace ascii = boost::spirit::ascii;
    //]

    ///////////////////////////////////////////////////////////////////////////
    //  Our mini XML tree representation
    ///////////////////////////////////////////////////////////////////////////
    struct mini_xml;

    typedef
        boost::variant<
            boost::recursive_wrapper<mini_xml>
          , std::string
        >
    mini_xml_node;

    struct mini_xml
    {
        std::string name;                           // tag name
        std::vector<mini_xml_node> children;        // children
    };
}

// We need to tell fusion about our mini_xml struct
// to make it a first-class fusion citizen
BOOST_FUSION_ADAPT_STRUCT(
    client::mini_xml,
    (std::string, name)
    (std::vector<client::mini_xml_node>, children)
)

namespace client
{
    ///////////////////////////////////////////////////////////////////////////
    //  Print out the mini xml tree
    ///////////////////////////////////////////////////////////////////////////
    int const tabsize = 4;

    void tab(int indent)
    {
        for (int i = 0; i < indent; ++i)
            std::cout << ' ';
    }

    struct mini_xml_printer
    {
        mini_xml_printer(int indent = 0)
          : indent(indent)
        {
        }

        void operator()(mini_xml const& xml) const;

        int indent;
    };

    struct mini_xml_node_printer : boost::static_visitor<>
    {
        mini_xml_node_printer(int indent = 0)
          : indent(indent)
        {
        }

        void operator()(mini_xml const& xml) const
        {
            mini_xml_printer(indent+tabsize)(xml);
        }

        void operator()(std::string const& text) const
        {
            tab(indent+tabsize);
            std::cout << "text: \"" << text << '"' << std::endl;
        }

        int indent;
    };

    void mini_xml_printer::operator()(mini_xml const& xml) const
    {
        tab(indent);
        std::cout << "tag: " << xml.name << std::endl;
        tab(indent);
        std::cout << '{' << std::endl;

        BOOST_FOREACH(mini_xml_node const& node, xml.children)
        {
            boost::apply_visitor(mini_xml_node_printer(indent), node);
        }

        tab(indent);
        std::cout << '}' << std::endl;
    }

    ///////////////////////////////////////////////////////////////////////////
    //  Our mini XML grammar definition
    ///////////////////////////////////////////////////////////////////////////
    //[mini_xml2_sr_grammar
    template <typename Iterator>
    struct mini_xml_grammar
      : qi::grammar<Iterator, mini_xml(), ascii::space_type>
    {
        mini_xml_grammar()
          : mini_xml_grammar::base_type(entry)
        {
            using qi::lit;
            using qi::lexeme;
            using ascii::char_;
            using ascii::string;
            using namespace qi::labels;

            entry %= (
                xml %=
                        start_tag[_a = _1]
                    >>  *node
                    >>  end_tag(_a)

              , node %= xml | text

              , text %= lexeme[+(char_ - '<')]

              , start_tag %=
                        '<'
                    >>  !lit('/')
                    >>  lexeme[+(char_ - '>')]
                    >>  '>'

              , end_tag %=
                        "</"
                    >>  string(_r1)
                    >>  '>'
            );
        }

        qi::rule<Iterator, mini_xml(), ascii::space_type> entry;

        repo::qi::subrule<0, mini_xml(), qi::locals<std::string> > xml;
        repo::qi::subrule<1, mini_xml_node()> node;
        repo::qi::subrule<2, std::string()> text;
        repo::qi::subrule<3, std::string()> start_tag;
        repo::qi::subrule<4, void(std::string)> end_tag;
    };
    //]
}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    char const* filename;
    if (argc > 1)
    {
        filename = argv[1];
    }
    else
    {
        std::cerr << "Error: No input file provided." << std::endl;
        return 1;
    }

    std::ifstream in(filename, std::ios_base::in);

    if (!in)
    {
        std::cerr << "Error: Could not open input file: "
            << filename << std::endl;
        return 1;
    }

    std::string storage; // We will read the contents here.
    in.unsetf(std::ios::skipws); // No white space skipping!
    std::copy(
        std::istream_iterator<char>(in),
        std::istream_iterator<char>(),
        std::back_inserter(storage));

    typedef client::mini_xml_grammar<std::string::const_iterator> mini_xml_grammar;
    mini_xml_grammar xml; // Our grammar
    client::mini_xml ast; // Our tree

    using boost::spirit::ascii::space;
    std::string::const_iterator iter = storage.begin();
    std::string::const_iterator end = storage.end();
    bool r = phrase_parse(iter, end, xml, space, ast);

    if (r && iter == end)
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing succeeded\n";
        std::cout << "-------------------------\n";
        client::mini_xml_printer printer;
        printer(ast);
        return 0;
    }
    else
    {
        std::string::const_iterator some = iter+30;
        std::string context(iter, (some>end)?end:some);
        std::cout << "-------------------------\n";
        std::cout << "Parsing failed\n";
        std::cout << "stopped at: \": " << context << "...\"\n";
        std::cout << "-------------------------\n";
        return 1;
    }
}


