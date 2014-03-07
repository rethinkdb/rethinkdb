//  Hannibal: partial C++ grammar to parse C++ type information
//  Copyright (c) 2005-2006 Danny Havenith
// 
//  Boost.Wave: A Standard compliant C++ preprocessor
//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  http://www.boost.org/
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define HANNIBAL_DUMP_PARSE_TREE 1
//#define HANNIBAL_TRACE_DECLARATIONS
//#define BOOST_SPIRIT_DEBUG

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_tree_to_xml.hpp>
#include <boost/program_options.hpp>

///////////////////////////////////////////////////////////////////////////////
//  Include Wave itself
#include <boost/wave.hpp>

///////////////////////////////////////////////////////////////////////////////
// Include the lexer stuff
#include <boost/wave/cpplexer/cpp_lex_token.hpp>    // token class
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp> // lexer class

///////////////////////////////////////////////////////////////////////////////
//  Include the Hannibal grammar
#include "translation_unit_parser.h"
#include "translation_unit_skipper.h"

using std::vector;
using std::string;
namespace po = boost::program_options;

#if HANNIBAL_DUMP_PARSE_TREE != 0
///////////////////////////////////////////////////////////////////////////////
namespace {

    ///////////////////////////////////////////////////////////////////////////
    //  helper routines needed to generate the parse tree XML dump
    typedef boost::wave::cpplexer::lex_token<> token_type;
    
    token_type::string_type  get_token_id(token_type const &t) 
    { 
        using namespace boost::wave;
        return get_token_name(token_id(t)); // boost::wave::token_id(t); 
    }
    
    token_type::string_type get_token_value(token_type const &t) 
    { 
        return t.get_value(); 
    }

///////////////////////////////////////////////////////////////////////////////
}   // unnamed namespace 
#endif

///////////////////////////////////////////////////////////////////////////////
namespace {
    
    ///////////////////////////////////////////////////////////////////////////
    // Parse the command line for input files and  (system-) include paths
    // Prints usage info if needed.
    // returns true if program should continue, false if program must stop
    bool parse_command_line( int argc, char *argv[], po::variables_map &vm)
    {
        //
        // Setup command line structure
        po::options_description visible("Usage: hannibal [options] file");
        visible.add_options()
                ("help,h", "show this help message")
                ("include,I", po::value<vector<string> >(), 
                    "specify additional include directory")
                ("sysinclude,S", po::value<vector<string> >(), 
                    "specify additional system include directory")
                ("define,D", po::value<vector<string> >()->composing(), 
                    "specify a macro to define (as macro[=[value]])")
                ("predefine,P", po::value<vector<string> >()->composing(), 
                    "specify a macro to predefine (as macro[=[value]])")
                ("undefine,U", po::value<vector<string> >()->composing(), 
                    "specify a macro to undefine")
            ;
            
        po::options_description hidden;
        hidden.add_options()
            ("input-file", "input file");

        po::options_description desc;
        desc.add( visible).add( hidden);

        po::positional_options_description p;
        p.add("input-file", 1);

        //
        // Parse
        po::store(po::command_line_parser(argc, argv).
                options(desc).positional(p).run(), vm);
        po::notify(vm);

        //
        // Print usage, if necessary
        if (!vm.count( "input-file") ||  vm.count( "help"))
        {
            std::cout << visible << std::endl;
            return false;
        }
        else 
        {
            return true;
        }
    }

///////////////////////////////////////////////////////////////////////////////
}   // unnamed namespace

///////////////////////////////////////////////////////////////////////////////
// main entry point
int main(int argc, char *argv[])
{
    po::variables_map vm;

    if (!parse_command_line( argc, argv, vm))
    {
        return -1;
    }

   string inputfile = vm["input-file"].as< string>();

// current file position is saved for exception handling
boost::wave::util::file_position_type current_position;

    try {
    //  Open and read in the specified input file.
        std::ifstream instream( inputfile.c_str());
        std::string instring;

        if (!instream.is_open()) {
            std::cerr << "Hannibal: could not open input file: " << inputfile
                      << std::endl;
            return -2;
        }
        instream.unsetf(std::ios::skipws);
        instring = std::string(std::istreambuf_iterator<char>(instream.rdbuf()),
                               std::istreambuf_iterator<char>());

    //  The template boost::wave::cpplexer::lex_token<> is the token type to be 
    //  used by the Wave library.
        typedef boost::wave::cpplexer::lex_token<> token_type;
    
    //  The template boost::wave::cpplexer::lex_iterator<> is the lexer type to
    //  be used by the Wave library.
        typedef boost::wave::cpplexer::lex_iterator<token_type> lex_iterator_type;

    //  This is the resulting context type to use. The first template parameter
    //  should match the iterator type to be used during construction of the
    //  corresponding context object (see below).
        typedef boost::wave::context<
                std::string::iterator, 
                lex_iterator_type,
                boost::wave::iteration_context_policies::load_file_to_string
            > context_type;

    // The preprocessor iterator shouldn't be constructed directly. It is 
    // to be generated through a wave::context<> object. This wave:context<> 
    // object is to be used additionally to initialize and define different 
    // parameters of the actual preprocessing (not done here).
    //
    // The preprocessing of the input stream is done on the fly behind the 
    // scenes during iteration over the context_type::iterator_type stream.
    context_type ctx (instring.begin(), instring.end(), inputfile.c_str());

    // add include directories to the include path
        if (vm.count("include")) {
            vector<string> const &paths = 
                vm["include"].as<vector<string> >();
            vector<string>::const_iterator end = paths.end();
            for (vector<string>::const_iterator cit = paths.begin(); 
                cit != end; ++cit)
            {
                ctx.add_include_path((*cit).c_str());
            }
        }

    // add system include directories to the include path
        if (vm.count("sysinclude")) {
            vector<string> const &syspaths = 
                vm["sysinclude"].as<vector<string> >();
            vector<string>::const_iterator end = syspaths.end();
            for (vector<string>::const_iterator cit = syspaths.begin(); 
                cit != end; ++cit)
            {
                ctx.add_sysinclude_path((*cit).c_str());
            }
        }

    // add additional defined macros 
        if (vm.count("define")) {
            vector<string> const &macros = vm["define"].as<vector<string> >();
            vector<string>::const_iterator end = macros.end();
            for (vector<string>::const_iterator cit = macros.begin(); 
                 cit != end; ++cit)
            {
                ctx.add_macro_definition(*cit);
            }
        }

    // add additional predefined macros 
        if (vm.count("predefine")) {
            vector<string> const &predefmacros = 
                vm["predefine"].as<vector<string> >();
            vector<string>::const_iterator end = predefmacros.end();
            for (vector<string>::const_iterator cit = predefmacros.begin(); 
                 cit != end; ++cit)
            {
                ctx.add_macro_definition(*cit, true);
            }
        }

    // undefine specified macros
        if (vm.count("undefine")) {
            vector<string> const &undefmacros = 
                vm["undefine"].as<vector<string> >();
            vector<string>::const_iterator end = undefmacros.end();
            for (vector<string>::const_iterator cit = undefmacros.begin(); 
                 cit != end; ++cit)
            {
                ctx.remove_macro_definition((*cit).c_str(), true);
            }
        }

    // analyze the input file
    context_type::iterator_type first = ctx.begin();
    context_type::iterator_type last = ctx.end();

    translation_unit_skipper s; 

#if HANNIBAL_DUMP_PARSE_TREE != 0
    typedef boost::spirit::classic::tree_parse_info<context_type::iterator_type> 
        result_type;
    translation_unit_grammar::rule_map_type rule_map;
    translation_unit_grammar g(&rule_map);

    // parse the input file
    result_type pi = boost::spirit::classic::ast_parse(first, last, g, s);
#else
    typedef boost::spirit::classic::parse_info<context_type::iterator_type> 
        result_type;
    translation_unit_grammar g;

    // parse the input file
    result_type pi = boost::spirit::classic::parse(first, last, g, s);
#endif

        if (pi.full) {
            std::cout 
                << "Hannibal: parsed sucessfully: " << inputfile << std::endl;

#if HANNIBAL_DUMP_PARSE_TREE != 0
            // generate xml dump from parse tree, if requested
            boost::spirit::classic::tree_to_xml(std::cerr, pi.trees, "", rule_map, 
                &get_token_id, &get_token_value);
#endif
        }
        else {
            std::cerr 
                << "Hannibal: parsing failed: " << inputfile << std::endl;
            std::cerr 
                << "Hannibal: last recognized token was: " 
                << get_token_id(*pi.stop) << std::endl;

            using boost::wave::util::file_position_type;
            file_position_type const& pos(pi.stop->get_position());
            std::cerr 
                << "Hannibal: at: " << pos.get_file() << "(" << pos.get_line() 
                << "," << pos.get_column() << ")" << std::endl;
            return 1;
        }
    }
    catch (boost::wave::cpp_exception const& e) {
    // some preprocessing error
        std::cerr 
            << e.file_name() << ":" << e.line_no() << ":" << e.column_no() 
            << ": " << e.description() << std::endl;
        return 2;
    }
    catch (boost::wave::cpplexer::lexing_exception const& e) {
    // some lexing error
        std::cerr 
            << e.file_name() << ":" << e.line_no() << ":" << e.column_no() 
            << ": " << e.description() << std::endl;
        return 2;
    }
    catch (std::exception const& e) {
    // use last recognized token to retrieve the error position
        std::cerr 
            << current_position.get_file() 
            << "(" << current_position.get_line() << "): "
            << "exception caught: " << e.what()
            << std::endl;
        return 3;
    }
    catch (...) {
    // use last recognized token to retrieve the error position
        std::cerr 
            << current_position.get_file() 
            << "(" << current_position.get_line() << "): "
            << "unexpected exception caught." << std::endl;
        return 4;
    }
    return 0;
}
