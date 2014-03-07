// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------

/* This is grammar of INFO file format written in form of boost::spirit rules.
   For simplicity, it does not parse #include directive. Note that INFO parser 
   included in property_tree library does not use Spirit.
*/

//#define BOOST_SPIRIT_DEBUG        // uncomment to enable debug output
#include <boost/spirit.hpp>

struct info_grammar: public boost::spirit::grammar<info_grammar>
{
    
    template<class Scanner>
    struct definition
    {
        
        boost::spirit::rule<typename boost::spirit::lexeme_scanner<Scanner>::type> chr, qchr, escape_seq;
        boost::spirit::rule<Scanner> string, qstring, cstring, key, value, entry, info;

        definition(const info_grammar &self)
        {

            using namespace boost::spirit;

            escape_seq = chset_p("0abfnrtv\"\'\\");
            chr = (anychar_p - space_p - '\\' - '{' - '}' - '#' - '"') | ('\\' >> escape_seq);
            qchr = (anychar_p - '"' - '\n' - '\\') | ('\\' >> escape_seq);
            string = lexeme_d[+chr];
            qstring = lexeme_d['"' >> *qchr >> '"'];
            cstring = lexeme_d['"' >> *qchr >> '"' >> '\\'];
            key = string | qstring;
            value = string | qstring | (+cstring >> qstring) | eps_p;
            entry = key >> value >> !('{' >> *entry >> '}');
            info = *entry >> end_p;

            // Debug nodes
            BOOST_SPIRIT_DEBUG_NODE(escape_seq);
            BOOST_SPIRIT_DEBUG_NODE(chr);
            BOOST_SPIRIT_DEBUG_NODE(qchr);
            BOOST_SPIRIT_DEBUG_NODE(string);
            BOOST_SPIRIT_DEBUG_NODE(qstring);
            BOOST_SPIRIT_DEBUG_NODE(key);
            BOOST_SPIRIT_DEBUG_NODE(value);
            BOOST_SPIRIT_DEBUG_NODE(entry);
            BOOST_SPIRIT_DEBUG_NODE(info);

        }

        const boost::spirit::rule<Scanner> &start() const
        {
            return info;
        }

    };
};

void info_parse(const char *s)
{

    using namespace boost::spirit;

    // Parse and display result
    info_grammar g;
    parse_info<const char *> pi = parse(s, g, space_p | comment_p(";"));
    std::cout << "Parse result: " << (pi.hit ? "Success" : "Failure") << "\n";
    
}

int main()
{

    // Sample data 1
    const char *data1 = 
        "\n"
        "key1 data1\n"
        "{\n"
        "\tkey data\n"
        "}\n"
        "key2 \"data2  \" {\n"
        "\tkey data\n"
        "}\n"
        "key3 \"data\"\n"
        "\t \"3\" {\n"
        "\tkey data\n"
        "}\n"
        "\n"
        "\"key4\" data4\n"
        "{\n"
        "\tkey data\n"
        "}\n"
        "\"key.5\" \"data.5\" { \n"
        "\tkey data \n"
        "}\n"
        "\"key6\" \"data\"\n"
        "\t   \"6\" {\n"
        "\tkey data\n"
        "}\n"
        "   \n"
        "key1 data1\n"
        "{\n"
        "\tkey data\n"
        "}\n"
        "key2 \"data2  \" {\n"
        "\tkey data\n"
        "}\n"
        "key3 \"data\"\n"
        "\t \"3\" {\n"
        "\tkey data\n"
        "}\n"
        "\n"
        "\"key4\" data4\n"
        "{\n"
        "\tkey data\n"
        "}\n"
        "\"key.5\" \"data.5\" {\n"
        "\tkey data\n"
        "}\n"
        "\"key6\" \"data\"\n"
        "\t   \"6\" {\n"
        "\tkey data\n"
        "}\n"
        "\\\\key\\t7 data7\\n\\\"data7\\\"\n"
        "{\n"
        "\tkey data\n"
        "}\n"
        "\"\\\\key\\t8\" \"data8\\n\\\"data8\\\"\"\n"
        "{\n"
        "\tkey data\n"
        "}\n"
        "\n";

    // Sample data 2
    const char *data2 = 
        "key1\n"
        "key2\n"
        "key3\n"
        "key4\n";

    // Parse sample data
    info_parse(data1);
    info_parse(data2);

}
