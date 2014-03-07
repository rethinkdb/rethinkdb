/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/repository/include/qi_kwd.hpp>
#include <boost/spirit/repository/include/qi_keywords.hpp>
#include <boost/optional.hpp>
#include <boost/cstdint.hpp>
#include <iostream>
#include <string>
#include <cstdlib>
#include <iterator>
#include <map>
#include <vector>

// Data structure definitions

// preprocessor constants
typedef std::pair<std::string, boost::int32_t> preprocessor_symbol; 

BOOST_FUSION_ADAPT_STRUCT( preprocessor_symbol,
    (std::string, first)
    (boost::int32_t, second)
)

// A data structure to store our program options
struct program_options {
    std::vector<std::string> includes; // include paths
    typedef std::vector< preprocessor_symbol > preprocessor_symbols_container; // symbol container type definition
    preprocessor_symbols_container preprocessor_symbols; // preprocessor symbols
    boost::optional<std::string> output_filename; // output file name
    std::string source_filename; // source file name
 
};

// Make the program_options compatible with fusion sequences
BOOST_FUSION_ADAPT_STRUCT( program_options,
    (std::vector<std::string>, includes)
    (program_options::preprocessor_symbols_container, preprocessor_symbols)
    (boost::optional<std::string>, output_filename)
    (std::string, source_filename)
)


// Output helper to check that the data parsed matches what we expect
std::ostream &operator<<(std::ostream &os, const program_options &obj)
{
    using boost::spirit::karma::string;
    using boost::spirit::karma::int_;
    using boost::spirit::karma::lit;
    using boost::spirit::karma::buffer;
    using boost::spirit::karma::eol;
    using boost::spirit::karma::format;
    return os<<format( 
                   lit("Includes:") << (string % ',') << eol
                << lit("Preprocessor symbols:") << ((string <<"="<< int_) % ',') << eol
                << buffer[-( lit("Output file:")<< string << eol)] 
                << lit("Source file:")<< string << eol
                ,obj);
    return os;
}


int
main()
{
    
    {
        // Pull everything we need from qi into this scope
        using boost::spirit::repository::qi::kwd;
        using boost::spirit::qi::inf;
        using boost::spirit::ascii::space_type; 
        using boost::spirit::ascii::alnum;
        using boost::spirit::qi::int_;
        using boost::spirit::qi::rule;
        using boost::spirit::qi::lit;
        using boost::spirit::qi::attr;
        using boost::spirit::qi::lexeme;
        using boost::spirit::qi::hold;
        using boost::spirit::qi::ascii::space;
       
        //Rule declarations
        rule<const char *, std::string(), space_type> parse_string;
        rule<const char *, program_options(), space_type> kwd_rule; 
       
        // A string parser 
        parse_string   %= lexeme[*alnum];
        
        namespace phx=boost::phoenix;    
        // kwd rule
        kwd_rule %= 
            kwd("--include")[ parse_string ]
          / kwd("--define")[ parse_string >> ((lit('=') > int_) | attr(1)) ]
          / kwd("--output",0,1)[ parse_string ]
          / hold [kwd("--source",1)[ parse_string ]]
          ;
        //
        
    using boost::spirit::qi::phrase_parse;

    // Let's check what that parser can do 
    program_options result;
    
    char const input[]="--include path1 --source file1 --define SYMBOL1=10 --include path2 --source file2";
    char const* f(input);
    char const* l(f + strlen(f));
    if (phrase_parse(f, l, kwd_rule, space,result) && (f == l))
        std::cout << "ok" << std::endl;
    else
        std::cout << "fail" << std::endl;

    // Output the result to the console
    std::cout<<result<<std::endl;
}        
    return 0;
}
