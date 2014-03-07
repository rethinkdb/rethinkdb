/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "token_statistics.hpp"                     // config data

///////////////////////////////////////////////////////////////////////////////
//  include required boost libraries
#include <boost/assert.hpp>
#include <boost/program_options.hpp>

///////////////////////////////////////////////////////////////////////////////
//  Include Wave itself
#include <boost/wave.hpp>

///////////////////////////////////////////////////////////////////////////////
// Include the lexer stuff
#include <boost/wave/cpplexer/cpp_lex_token.hpp>    // token class
#include "xlex_iterator.hpp"                        // lexer class

#include "collect_token_statistics.hpp"

///////////////////////////////////////////////////////////////////////////////
//  import required names
using namespace boost::spirit::classic;

using std::string;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
using std::ostream;
using std::istreambuf_iterator;

namespace po = boost::program_options;

///////////////////////////////////////////////////////////////////////////////
namespace cmd_line_util {

    // predicate to extract all positional arguments from the command line
    struct is_argument {
    
        bool operator()(po::option const &opt)
        {
            return (opt.position_key == -1) ? true : false;
        }
    };
    
///////////////////////////////////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////
// print the current version

int print_version()
{
// get time of last compilation of this file
boost::wave::util::time_conversion_helper compilation_time(__DATE__ " " __TIME__);

// calculate the number of days since May 9 2005
// (the day the token_statistics project was started)
std::tm first_day;

    std::memset (&first_day, 0, sizeof(std::tm));
    first_day.tm_mon = 4;           // May
    first_day.tm_mday = 9;          // 09
    first_day.tm_year = 105;        // 2005

long seconds = long(std::difftime(compilation_time.get_time(), 
    std::mktime(&first_day)));

    cout 
        << TOKEN_STATISTICS_VERSION_MAJOR << '.' 
        << TOKEN_STATISTICS_VERSION_MINOR << '.'
        << TOKEN_STATISTICS_VERSION_SUBMINOR << '.'
        << seconds/(3600*24);       // get number of days from seconds
    return 1;                       // exit app
}

///////////////////////////////////////////////////////////////////////////////
//  
int 
do_actual_work(vector<string> const &arguments, po::variables_map const &vm)
{
// current file position is saved for exception handling
boost::wave::util::file_position_type current_position;

    try {
    // this object keeps track of all the statistics
        collect_token_statistics stats;
        
    // collect the token statistics for all arguments given
        vector<string>::const_iterator lastfile = arguments.end();
        for (vector<string>::const_iterator file_it = arguments.begin(); 
             file_it != lastfile; ++file_it)
        {
        ifstream instream((*file_it).c_str());
        string instring;

            if (!instream.is_open()) {
                cerr << "token_statistics: could not open input file: " 
                     << *file_it << endl;
                continue;
            }
            instream.unsetf(std::ios::skipws);
            instring = string(istreambuf_iterator<char>(instream.rdbuf()),
                              istreambuf_iterator<char>());
            
        //  The template boost::wave::cpplexer::lex_token<> is the token type to be 
        //  used by the Wave library.
            typedef boost::wave::cpplexer::xlex::xlex_iterator<
                    boost::wave::cpplexer::lex_token<> >
                lexer_type;
            typedef boost::wave::context<
                    std::string::iterator, lexer_type
                > context_type;

        // The preprocessor iterator shouldn't be constructed directly. It is 
        // to be generated through a wave::context<> object. This wave:context<> 
        // object is additionally to be used to initialize and define different 
        // parameters of the actual preprocessing.
        // The preprocessing of the input stream is done on the fly behind the 
        // scenes during iteration over the context_type::iterator_type stream.
        context_type ctx (instring.begin(), instring.end(), (*file_it).c_str());

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
            
        // analyze the actual file
        context_type::iterator_type first = ctx.begin();
        context_type::iterator_type last = ctx.end();
        
            while (first != last) {
                current_position = (*first).get_position();
                stats(*first);
                ++first;
            }
        }
        
    // print out the collected statistics
        stats.print();
    }
    catch (boost::wave::cpp_exception const& e) {
    // some preprocessing error
        cerr 
            << e.file_name() << "(" << e.line_no() << "): "
            << e.description() << endl;
        return 2;
    }
    catch (std::exception const& e) {
    // use last recognized token to retrieve the error position
        cerr 
            << current_position.get_file() 
            << "(" << current_position.get_line() << "): "
            << "exception caught: " << e.what()
            << endl;
        return 3;
    }
    catch (...) {
    // use last recognized token to retrieve the error position
        cerr 
            << current_position.get_file() 
            << "(" << current_position.get_line() << "): "
            << "unexpected exception caught." << endl;
        return 4;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//  here we go!
int
main (int argc, char *argv[])
{
    try {
    // analyze the command line options and arguments
    vector<string> syspathes;
    po::options_description desc("Usage: token_statistics [options] file ...");
        
        desc.add_options()
            ("help,h", "print out program usage (this message)")
            ("version,v", "print the version number")
            ("include,I", po::value<vector<string> >(), 
                "specify additional include directory")
            ("sysinclude,S", po::value<vector<string> >(), 
                "specify additional system include directory")
        ;

        using namespace boost::program_options::command_line_style;

    po::parsed_options opts = po::parse_command_line(argc, argv, desc, unix_style);
    po::variables_map vm;
    
        po::store(opts, vm);
        po::notify(vm);
        
        if (vm.count("help")) {
            cout << desc << endl;
            return 1;
        }
        
        if (vm.count("version")) {
            return print_version();
        }

    // extract the arguments from the parsed command line
    vector<po::option> arguments;
    
        std::remove_copy_if(opts.options.begin(), opts.options.end(), 
            inserter(arguments, arguments.end()), cmd_line_util::is_argument());

    // if there is no input file given, then exit
        if (0 == arguments.size() || 0 == arguments[0].value.size()) {
            cerr << "token_statistics: No input file given. "
                 << "Use --help to get a hint." << endl;
            return 5;
        }

    // iterate over all given input files
        return do_actual_work(arguments[0].value , vm);
    }
    catch (std::exception const& e) {
        cout << "token_statistics: exception caught: " << e.what() << endl;
        return 6;
    }
    catch (...) {
        cerr << "token_statistics: unexpected exception caught." << endl;
        return 7;
    }
}

