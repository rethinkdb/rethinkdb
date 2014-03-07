/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Sample: IDL oriented preprocessor
    
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "idl.hpp"                  // global configuration

#include <boost/assert.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem/path.hpp>

///////////////////////////////////////////////////////////////////////////////
//  Include Wave itself
#include <boost/wave.hpp>

///////////////////////////////////////////////////////////////////////////////
//  Include the lexer related stuff
#include <boost/wave/cpplexer/cpp_lex_token.hpp>  // token type
#include "idllexer/idl_lex_iterator.hpp"          // lexer type

///////////////////////////////////////////////////////////////////////////////
//  include lexer specifics, import lexer names
//
#if BOOST_WAVE_SEPARATE_LEXER_INSTANTIATION == 0
#include "idllexer/idl_re2c_lexer.hpp"
#endif 

///////////////////////////////////////////////////////////////////////////////
//  include the grammar definitions, if these shouldn't be compiled separately
//  (ATTENTION: _very_ large compilation times!)
//
#if BOOST_WAVE_SEPARATE_GRAMMAR_INSTANTIATION == 0
#include <boost/wave/grammars/cpp_intlit_grammar.hpp>
#include <boost/wave/grammars/cpp_chlit_grammar.hpp>
#include <boost/wave/grammars/cpp_grammar.hpp>
#include <boost/wave/grammars/cpp_expression_grammar.hpp>
#include <boost/wave/grammars/cpp_predef_macros_grammar.hpp>
#include <boost/wave/grammars/cpp_defined_grammar.hpp>
#endif 

///////////////////////////////////////////////////////////////////////////////
//  import required names
using namespace boost::spirit::classic;

using std::string;
using std::pair;
using std::vector;
using std::getline;
using std::ifstream;
using std::cout;
using std::cerr;
using std::endl;
using std::ostream;
using std::istreambuf_iterator;

namespace po = boost::program_options;
namespace fs = boost::filesystem;

///////////////////////////////////////////////////////////////////////////////
// print the current version
int print_version()
{
    typedef boost::wave::idllexer::lex_iterator<
            boost::wave::cpplexer::lex_token<> >
        lex_iterator_type;
    typedef boost::wave::context<std::string::iterator, lex_iterator_type>
        context_type;
        
    string version (context_type::get_version_string());
    cout 
        << version.substr(1, version.size()-2)  // strip quotes
        << " (" << IDL_VERSION_DATE << ")"      // add date
        << endl;
    return 0;                       // exit app
}

///////////////////////////////////////////////////////////////////////////////
// print the copyright statement
int print_copyright()
{
    char const *copyright[] = {
        "",
        "Sample: IDL oriented preprocessor",
        "Based on: Wave, A Standard conformant C++ preprocessor library",
        "It is hosted by http://www.boost.org/.", 
        "",
        "Copyright (c) 2001-2010 Hartmut Kaiser, Distributed under the Boost",
        "Software License, Version 1.0. (See accompanying file",
        "LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)",
        0
    };
    
    for (int i = 0; 0 != copyright[i]; ++i)
        cout << copyright[i] << endl;
        
    return 0;                       // exit app
}

///////////////////////////////////////////////////////////////////////////////
namespace cmd_line_util {

    // Additional command line parser which interprets '@something' as an 
    // option "config-file" with the value "something".
    pair<string, string> at_option_parser(string const&s)
    {
        if ('@' == s[0]) 
            return std::make_pair(string("config-file"), s.substr(1));
        else
            return pair<string, string>();
    }

    // class, which keeps include file information read from the command line
    class include_paths {
    public:
        include_paths() : seen_separator(false) {}

        vector<string> paths;       // stores user paths
        vector<string> syspaths;    // stores system paths
        bool seen_separator;        // command line contains a '-I-' option

        // Function which validates additional tokens from command line.
        static void 
        validate(boost::any &v, vector<string> const &tokens)
        {
            if (v.empty())
                v = boost::any(include_paths());

            include_paths *p = boost::any_cast<include_paths>(&v);

            BOOST_ASSERT(p);
            // Assume only one path per '-I' occurrence.
            string t = tokens[0];
            if (t == "-") {
            // found -I- option, so switch behaviour
                p->seen_separator = true;
            } 
            else if (p->seen_separator) {
            // store this path as a system path
                p->syspaths.push_back(t); 
            } 
            else {
            // store this path as an user path
                p->paths.push_back(t);
            }            
        }
    };

    // Read all options from a given config file, parse and add them to the
    // given variables_map
    void read_config_file_options(string const &filename, 
        po::options_description const &desc, po::variables_map &vm,
        bool may_fail = false)
    {
    ifstream ifs(filename.c_str());

        if (!ifs.is_open()) {
            if (!may_fail) {
                cerr << filename 
                    << ": command line warning: config file not found"
                    << endl;
            }
            return;
        }
        
    vector<string> options;
    string line;

        while (std::getline(ifs, line)) {
        // skip empty lines
            string::size_type pos = line.find_first_not_of(" \t");
            if (pos == string::npos) 
                continue;

        // skip comment lines
            if ('#' != line[pos])
                options.push_back(line);
        }

        if (options.size() > 0) {
            using namespace boost::program_options::command_line_style;
            po::store(po::command_line_parser(options)
                .options(desc).style(unix_style).run(), vm);
            po::notify(vm);
        }
    }

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
//
//  Special validator overload, which allows to handle the -I- syntax for
//  switching the semantics of an -I option.
//
///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace program_options {

  void validate(boost::any &v, std::vector<std::string> const &s,
      cmd_line_util::include_paths *, int) 
  {
      cmd_line_util::include_paths::validate(v, s);
  }

}}  // namespace boost::program_options

///////////////////////////////////////////////////////////////////////////////
//  do the actual preprocessing
int 
do_actual_work (std::string file_name, po::variables_map const &vm)
{
// current file position is saved for exception handling
boost::wave::util::file_position_type current_position;

    try {
    // process the given file
    ifstream instream(file_name.c_str());
    string instring;

        if (!instream.is_open()) {
            cerr << "waveidl: could not open input file: " << file_name << endl;
            return -1;
        }
        instream.unsetf(std::ios::skipws);
        
#if defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)
        // this is known to be very slow for large files on some systems
        copy (istream_iterator<char>(instream),
              istream_iterator<char>(), 
              inserter(instring, instring.end()));
#else
        instring = string(istreambuf_iterator<char>(instream.rdbuf()),
                          istreambuf_iterator<char>());
#endif 

    //  This sample uses the lex_token type predefined in the Wave library, but 
    //  but uses a custom lexer type.
        typedef boost::wave::idllexer::lex_iterator<
                boost::wave::cpplexer::lex_token<> >
            lex_iterator_type;
        typedef boost::wave::context<std::string::iterator, lex_iterator_type> 
            context_type;

    // The C++ preprocessor iterators shouldn't be constructed directly. They 
    // are to be generated through a boost::wave::context<> object. This 
    // boost::wave::context object is additionally to be used to initialize and 
    // define different parameters of the actual preprocessing.
    // The preprocessing of the input stream is done on the fly behind the 
    // scenes during iteration over the context_type::iterator_type stream.
    context_type ctx (instring.begin(), instring.end(), file_name.c_str());

    // add include directories to the system include search paths
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
        
    // add include directories to the include search paths
        if (vm.count("include")) {
            cmd_line_util::include_paths const &ip = 
                vm["include"].as<cmd_line_util::include_paths>();
            vector<string>::const_iterator end = ip.paths.end();

            for (vector<string>::const_iterator cit = ip.paths.begin(); 
                 cit != end; ++cit)
            {
                ctx.add_include_path((*cit).c_str());
            }

        // if on the command line was given -I- , this has to be propagated
            if (ip.seen_separator) 
                ctx.set_sysinclude_delimiter();
                 
        // add system include directories to the include path
            vector<string>::const_iterator sysend = ip.syspaths.end();
            for (vector<string>::const_iterator syscit = ip.syspaths.begin(); 
                 syscit != sysend; ++syscit)
            {
                ctx.add_sysinclude_path((*syscit).c_str());
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

    // open the output file
    std::ofstream output;
    
        if (vm.count("output")) {
        // try to open the file, where to put the preprocessed output
        string out_file (vm["output"].as<string>());
        
            output.open(out_file.c_str());
            if (!output.is_open()) {
                cerr << "waveidl: could not open output file: " << out_file 
                     << endl;
                return -1;
            }
        }
        else {
        // output the preprocessed result to std::cout
            output.copyfmt(cout);
            output.clear(cout.rdstate());
            static_cast<std::basic_ios<char> &>(output).rdbuf(cout.rdbuf());
        }

    // analyze the input file
    context_type::iterator_type first = ctx.begin();
    context_type::iterator_type last = ctx.end();
    
    // loop over all generated tokens outputing the generated text 
        while (first != last) {
        // print out the string representation of this token (skip comments)
            using namespace boost::wave;

        // store the last known good token position
            current_position = (*first).get_position();

        token_id id = token_id(*first);

            if (T_CPPCOMMENT == id || T_NEWLINE == id) {
            // C++ comment tokens contain the trailing newline
                output << endl;
            }
            else if (id != T_CCOMMENT) {
            // print out the current token value
                output << (*first).get_value();
            }
            ++first;        // advance to the next token
        }
    }
    catch (boost::wave::cpp_exception const& e) {
    // some preprocessing error
        cerr 
            << e.file_name() << "(" << e.line_no() << "): "
            << e.description() << endl;
        return 1;
    }
    catch (boost::wave::cpplexer::lexing_exception const& e) {
    // some lexing error
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
//  main entry point
int
main (int argc, char *argv[])
{
    try {
    // analyze the command line options and arguments
    
    // declare the options allowed from the command line only
    po::options_description desc_cmdline ("Options allowed on the command line only");
        
        desc_cmdline.add_options()
            ("help,h", "print out program usage (this message)")
            ("version,v", "print the version number")
            ("copyright,c", "print out the copyright statement")
            ("config-file", po::value<vector<string> >(), 
                "specify a config file (alternatively: @filepath)")
        ;

    // declare the options allowed on command line and in config files
    po::options_description desc_generic ("Options allowed additionally in a config file");

        desc_generic.add_options()
            ("output,o", "specify a file to use for output instead of stdout")
            ("include,I", po::value<cmd_line_util::include_paths>()->composing(), 
                "specify an additional include directory")
            ("sysinclude,S", po::value<vector<string> >()->composing(), 
                "specify an additional system include directory")
            ("define,D", po::value<vector<string> >()->composing(), 
                "specify a macro to define (as macro[=[value]])")
            ("predefine,P", po::value<vector<string> >()->composing(), 
                "specify a macro to predefine (as macro[=[value]])")
            ("undefine,U", po::value<vector<string> >()->composing(), 
                "specify a macro to undefine")
        ;
        
    // combine the options for the different usage schemes
    po::options_description desc_overall_cmdline;
    po::options_description desc_overall_cfgfile;

        desc_overall_cmdline.add(desc_cmdline).add(desc_generic);
        desc_overall_cfgfile.add(desc_generic);
        
    // parse command line and store results
        using namespace boost::program_options::command_line_style;

    po::parsed_options opts = po::parse_command_line(argc, argv, 
        desc_overall_cmdline, unix_style, cmd_line_util::at_option_parser);
    po::variables_map vm;
    
        po::store(opts, vm);
        po::notify(vm);
        
    // Try to find a waveidl.cfg in the same directory as the executable was 
    // started from. If this exists, treat it as a wave config file
    fs::path filename(argv[0], fs::native);

        filename = filename.branch_path() / "waveidl.cfg";
        cmd_line_util::read_config_file_options(filename.string(), 
            desc_overall_cfgfile, vm, true);

    // if there is specified at least one config file, parse it and add the 
    // options to the main variables_map
        if (vm.count("config-file")) {
            vector<string> const &cfg_files = 
                vm["config-file"].as<vector<string> >();
            vector<string>::const_iterator end = cfg_files.end();
            for (vector<string>::const_iterator cit = cfg_files.begin(); 
                 cit != end; ++cit)
            {
            // parse a single config file and store the results
                cmd_line_util::read_config_file_options(*cit, 
                    desc_overall_cfgfile, vm);
            }
        }

    // ... act as required 
        if (vm.count("help")) {
        po::options_description desc_help (
            "Usage: waveidl [options] [@config-file(s)] file");

            desc_help.add(desc_cmdline).add(desc_generic);
            cout << desc_help << endl;
            return 1;
        }
        
        if (vm.count("version")) {
            return print_version();
        }

        if (vm.count("copyright")) {
            return print_copyright();
        }
        
    // extract the arguments from the parsed command line
    vector<po::option> arguments;
    
        std::remove_copy_if(opts.options.begin(), opts.options.end(), 
            inserter(arguments, arguments.end()), cmd_line_util::is_argument());
            
    // if there is no input file given, then exit
        if (0 == arguments.size() || 0 == arguments[0].value.size()) {
            cerr << "waveidl: no input file given, "
                 << "use --help to get a hint." << endl;
            return 5;
        }

    // preprocess the given input file
        return do_actual_work(arguments[0].value[0], vm);
    }
    catch (std::exception const& e) {
        cout << "waveidl: exception caught: " << e.what() << endl;
        return 6;
    }
    catch (...) {
        cerr << "waveidl: unexpected exception caught." << endl;
        return 7;
    }
}

