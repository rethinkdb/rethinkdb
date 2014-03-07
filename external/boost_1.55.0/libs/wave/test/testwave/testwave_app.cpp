/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2013 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// disable stupid compiler warnings
#include <boost/config/warning_disable.hpp>

// system headers
#include <string>
#include <iostream>
#include <vector>
#include <ctime>

// include boost
#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/detail/workaround.hpp>

//  include Wave

// always use new hooks
#define BOOST_WAVE_USE_DEPRECIATED_PREPROCESSING_HOOKS 0

#include <boost/wave.hpp>

//  include the lexer related stuff
#include <boost/wave/cpplexer/cpp_lex_token.hpp>      // token type
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>   // lexer type

//  test application related headers
#include "cmd_line_utils.hpp"
#include "testwave_app.hpp"
#include "collect_hooks_information.hpp"

# ifdef BOOST_NO_STDC_NAMESPACE
namespace std
{
    using ::asctime; using ::gmtime; using ::localtime;
    using ::difftime; using ::time; using ::tm; using ::mktime; using ::system;
}
# endif

namespace po = boost::program_options;
namespace fs = boost::filesystem;

///////////////////////////////////////////////////////////////////////////////
// testwave version definitions
#define TESTWAVE_VERSION_MAJOR           0
#define TESTWAVE_VERSION_MINOR           6
#define TESTWAVE_VERSION_SUBMINOR        0

namespace {

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    inline bool
    handle_next_token(Iterator &it, Iterator const& end,
        std::string &result)
    {
        typedef typename Iterator::value_type token_type;

        token_type tok = *it++;
        result = result + tok.get_value().c_str();
        return (it == end) ? false : true;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename String>
    String const& handle_quoted_filepath(String &name)
    {
        using boost::wave::util::impl::unescape_lit;

        String unesc_name = unescape_lit(name.substr(1, name.size()-2));
        fs::path p (boost::wave::util::create_path(unesc_name.c_str()));

        name = String("\"") + boost::wave::util::leaf(p).c_str() + String("\"");
        return name;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    bool handle_line_directive(Iterator &it, Iterator const& end,
        std::string &result)
    {
        typedef typename Iterator::value_type token_type;
        typedef typename token_type::string_type string_type;

        if (!handle_next_token(it, end, result) ||  // #line
            !handle_next_token(it, end, result) ||  // whitespace
            !handle_next_token(it, end, result) ||  // number
            !handle_next_token(it, end, result))    // whitespace
        {
            return false;
        }

        using boost::wave::util::impl::unescape_lit;

        token_type filename = *it;
        string_type name = filename.get_value();

        handle_quoted_filepath(name);
        result = result + name.c_str();
        return true;
    }

    template <typename T>
    inline T const&
    variables_map_as(po::variable_value const& v, T*)
    {
#if (__GNUC__ == 3 && (__GNUC_MINOR__ == 2 || __GNUC_MINOR__ == 3)) || \
    BOOST_WORKAROUND(__MWERKS__, < 0x3200)
// gcc 3.2.x and  3.3.x choke on vm[...].as<...>()
// CW 8.3 has problems with the v.as<T>() below
        T const* r = boost::any_cast<T>(&v.value());
        if (!r)
            boost::throw_exception(boost::bad_any_cast());
        return *r;
#else
        return v.as<T>();
#endif
    }

}

///////////////////////////////////////////////////////////////////////////
//
//  This function compares the real result and the expected one but first
//  replaces all occurrences in the expected result of
//      $E: to the result of preprocessing the given expression
//      $F: to the passed full filepath
//      $P: to the full path
//      $B: to the full path (same as $P, but using forward slash '/' on Windows)
//      $V: to the current Boost version number
//
///////////////////////////////////////////////////////////////////////////
bool
testwave_app::got_expected_result(std::string const& filename,
    std::string const& result, std::string& expected)
{
    using boost::wave::util::impl::escape_lit;

    std::string full_result;
    std::string::size_type pos = 0;
    std::string::size_type pos1 = expected.find_first_of("$");

    if (pos1 != std::string::npos) {
        do {
            switch(expected[pos1+1]) {
            case 'E':       // preprocess the given token sequence
                {
                    if ('(' == expected[pos1+2]) {
                        std::size_t p = expected.find_first_of(")", pos1+1);
                        if (std::string::npos == p) {
                            std::cerr
                                << "testwave: unmatched parenthesis in $E"
                                    " directive" << std::endl;
                            return false;
                        }
                        std::string source = expected.substr(pos1+3, p-pos1-3);
                        std::string result, error, hooks;
                        bool pp_result = preprocess_file(filename, source,
                            result, error, hooks, true);
                        if (!pp_result) {
                            std::cerr
                                << "testwave: preprocessing error in $E directive: "
                                << error << std::endl;
                            return false;
                        }
                        full_result = full_result +
                            expected.substr(pos, pos1-pos) + result;
                        pos1 = expected.find_first_of ("$",
                            pos = pos1 + 4 + source.size());
                    }
                }
                break;

            case 'F':       // insert base file name
                full_result = full_result +
                    expected.substr(pos, pos1-pos) + escape_lit(filename);
                pos1 = expected.find_first_of ("$", pos = pos1 + 2);
                break;

            case 'P':       // insert full path
            case 'B':       // same as 'P', but forward slashes on Windows
                {
                    fs::path fullpath (
                        boost::wave::util::complete_path(
                            boost::wave::util::create_path(filename),
                            boost::wave::util::current_path())
                        );

                    if ('(' == expected[pos1+2]) {
                    // the $P(basename) syntax is used
                        std::size_t p = expected.find_first_of(")", pos1+1);
                        if (std::string::npos == p) {
                            std::cerr
                                << "testwave: unmatched parenthesis in $P"
                                    " directive" << std::endl;
                            return false;
                        }
                        std::string base = expected.substr(pos1+3, p-pos1-3);
                        fullpath = boost::wave::util::branch_path(fullpath) /
                            boost::wave::util::create_path(base);
                        full_result += expected.substr(pos, pos1-pos);
                        if ('P' == expected[pos1+1]) {
#if defined(BOOST_WINDOWS)
                            std::string p = replace_slashes(
                                boost::wave::util::native_file_string(
                                    boost::wave::util::normalize(fullpath)),
                                "/", '\\');
#else
                            std::string p (
                                boost::wave::util::native_file_string(
                                    boost::wave::util::normalize(fullpath)));
#endif
                            full_result += escape_lit(p);
                        }
                        else {
#if defined(BOOST_WINDOWS)
                            std::string p = replace_slashes(
                                boost::wave::util::normalize(fullpath).string());
#else
                            std::string p (
                                boost::wave::util::normalize(fullpath).string());
#endif
                            full_result += escape_lit(p);
                        }
                        pos1 = expected.find_first_of ("$",
                            pos = pos1 + 4 + base.size());
                    }
                    else {
                    // the $P is used on its own
                        full_result += expected.substr(pos, pos1-pos);
                        if ('P' == expected[pos1+1]) {
                            full_result += escape_lit(
                                boost::wave::util::native_file_string(fullpath));
                        }
                        else {
#if defined(BOOST_WINDOWS)
                            std::string p = replace_slashes(fullpath.string());
#else
                            std::string p (fullpath.string());
#endif
                            full_result += escape_lit(fullpath.string());
                        }
                        pos1 = expected.find_first_of ("$", pos = pos1 + 2);
                    }
                }
                break;

            case 'R':       // insert relative file name
            case 'S':       // same as 'R', but forward slashes on Windows
                {
                    fs::path relpath;
                    boost::wave::util::as_relative_to(
                        boost::wave::util::create_path(filename),
                        boost::wave::util::current_path(),
                        relpath);

                    if ('(' == expected[pos1+2]) {
                    // the $R(basename) syntax is used
                        std::size_t p = expected.find_first_of(")", pos1+1);
                        if (std::string::npos == p) {
                            std::cerr
                                << "testwave: unmatched parenthesis in $R"
                                    " directive" << std::endl;
                            return false;
                        }
                        std::string base = expected.substr(pos1+3, p-pos1-3);
                        relpath = boost::wave::util::branch_path(relpath) /
                            boost::wave::util::create_path(base);
                        full_result += expected.substr(pos, pos1-pos);
                        if ('R' == expected[pos1+1]) {
                            full_result += escape_lit(
                                boost::wave::util::native_file_string(
                                    boost::wave::util::normalize(relpath)));
                        }
                        else {
#if defined(BOOST_WINDOWS)
                            std::string p = replace_slashes(
                                boost::wave::util::normalize(relpath).string());
#else
                            std::string p (
                                boost::wave::util::normalize(relpath).string());
#endif
                            full_result += escape_lit(p);
                        }
                        pos1 = expected.find_first_of ("$",
                            pos = pos1 + 4 + base.size());
                    }
                    else {
                    // the $R is used on its own
                        full_result += expected.substr(pos, pos1-pos);
                        if ('R' == expected[pos1+1]) {
                            full_result += escape_lit(
                                boost::wave::util::native_file_string(relpath));
                        }
                        else {
#if defined(BOOST_WINDOWS)
                            std::string p = replace_slashes(relpath.string());
#else
                            std::string p (relpath.string());
#endif
                            full_result += escape_lit(p);
                        }
                        pos1 = expected.find_first_of ("$", pos = pos1 + 2);
                    }
                }
                break;

            case 'V':       // insert Boost version
                full_result = full_result +
                    expected.substr(pos, pos1-pos) + BOOST_LIB_VERSION;
                pos1 = expected.find_first_of ("$", pos = pos1 + 2);
                break;

            default:
                full_result = full_result +
                    expected.substr(pos, pos1-pos);
                pos1 = expected.find_first_of ("$", (pos = pos1) + 1);
                break;
            }

        } while(pos1 != std::string::npos);
        full_result += expected.substr(pos);
    }
    else {
        full_result = expected;
    }

    expected = full_result;
    return full_result == result;
}

///////////////////////////////////////////////////////////////////////////////
testwave_app::testwave_app(po::variables_map const& vm)
:   debuglevel(1), desc_options("Preprocessor configuration options"),
    global_vm(vm)
{
    desc_options.add_options()
        ("include,I", po::value<cmd_line_utils::include_paths>()->composing(),
            "specify an additional include directory")
        ("sysinclude,S", po::value<std::vector<std::string> >()->composing(),
            "specify an additional system include directory")
        ("forceinclude,F", po::value<std::vector<std::string> >()->composing(),
            "force inclusion of the given file")
        ("define,D", po::value<std::vector<std::string> >()->composing(),
            "specify a macro to define (as macro[=[value]])")
        ("predefine,P", po::value<std::vector<std::string> >()->composing(),
            "specify a macro to predefine (as macro[=[value]])")
        ("undefine,U", po::value<std::vector<std::string> >()->composing(),
            "specify a macro to undefine")
        ("nesting,n", po::value<int>(),
            "specify a new maximal include nesting depth")
        ("long_long", "enable long long support in C++ mode")
        ("preserve", "preserve comments")
#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
        ("variadics", "enable certain C99 extensions in C++ mode")
        ("c99", "enable C99 mode (implies --variadics)")
#endif
#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
        ("noguard,G", "disable include guard detection")
#endif
        ("skipped_token_hooks", "record skipped_token hook calls")
#if BOOST_WAVE_SUPPORT_CPP0X != 0
        ("c++11", "enable C++11 mode (implies --variadics and --long_long)")
#endif
    ;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Test the given file (i.e. preprocess the file and compare the result
//  against the embedded 'R' comments, if an error occurs compare the error
//  message against the given 'E' comments, if no error occurred, compare the
//  generated hooks result against the given 'H' comments).
//
///////////////////////////////////////////////////////////////////////////////
bool
testwave_app::test_a_file(std::string filename)
{
// read the input file into a string
    std::string instr;
    if (!read_file(filename, instr))
        return false;     // error was reported already

    bool test_hooks = true;
    if (global_vm.count("hooks"))
        test_hooks = variables_map_as(global_vm["hooks"], (bool *)NULL);

// extract expected output, preprocess the data and compare results
    std::string expected, expected_hooks;
    if (extract_expected_output(filename, instr, expected, expected_hooks)) {
        bool retval = true;   // assume success
        bool printed_result = false;
        std::string result, error, hooks;
        bool pp_result = preprocess_file(filename, instr, result, error, hooks);
        if (pp_result || !result.empty()) {
        // did we expect an error?
            std::string expected_error;
            if (!extract_special_information(filename, instr, 'E', expected_error))
                return false;

            if (!expected_error.empty() &&
                !got_expected_result(filename, error, expected_error))
            {
            // we expected an error but got none (or a different one)
                if (debuglevel > 2) {
                    std::cerr
                        << filename << ": failed" << std::endl
                        << "result: " << std::endl << result << std::endl;

                    if (!error.empty()) {
                        std::cerr << "expected result: " << std::endl
                                  << expected << std::endl;
                    }
                    if (!expected_error.empty()) {
                        std::cerr << "expected error: " << std::endl
                                  << expected_error << std::endl;
                    }
                }
                else if (debuglevel > 1) {
                    std::cerr << filename << ": failed" << std::endl;
                }
                retval = false;
            }
            else if (!got_expected_result(filename, result, expected)) {
            //  no preprocessing error encountered
                if (debuglevel > 2) {
                    std::cerr
                        << filename << ": failed" << std::endl
                        << "result: " << std::endl << result << std::endl
                        << "expected: " << std::endl << expected << std::endl;
                }
                else if (debuglevel > 1) {
                    std::cerr << filename << ": failed" << std::endl;
                }
                retval = false;
            }
            else {
            // preprocessing succeeded, check hook information, if appropriate
                if (test_hooks && !expected_hooks.empty() &&
                    !got_expected_result(filename, hooks, expected_hooks))
                {
                    if (debuglevel > 2) {
                        std::cerr << filename << ": failed" << std::endl
                                  << "hooks result: " << std::endl << hooks
                                  << std::endl;
                        std::cerr << "expected hooks result: " << std::endl
                                  << expected_hooks << std::endl;
                    }
                    else if (debuglevel > 1) {
                        std::cerr << filename << ": failed" << std::endl;
                    }
                    retval = false;
                }
            }

            // print success message, if appropriate
            if (retval) {
                if (debuglevel > 5) {
                    std::cerr
                        << filename << ": succeeded" << std::endl
                        << "result: " << std::endl << result << std::endl
                        << "hooks result: " << std::endl << hooks << std::endl;
                }
                else if (debuglevel > 4) {
                    std::cerr
                        << filename << ": succeeded" << std::endl
                        << "result: " << std::endl << result << std::endl;
                }
                else if (debuglevel > 3) {
                    std::cerr << filename << ": succeeded" << std::endl;
                }
                printed_result = true;
            }
        }

        if (!pp_result) {
        //  there was a preprocessing error, was it expected?
            std::string expected_error;
            if (!extract_special_information(filename, instr, 'E', expected_error))
                return false;

            if (!got_expected_result(filename, error, expected_error)) {
            // the error was unexpected
                if (debuglevel > 2) {
                    std::cerr
                        << filename << ": failed" << std::endl;

                    if (!expected_error.empty()) {
                        std::cerr
                            << "error result: " << std::endl << error << std::endl
                            << "expected error: " << std::endl
                            << expected_error << std::endl;
                    }
                    else {
                        std::cerr << "unexpected error: " << error << std::endl;
                    }
                }
                else if (debuglevel > 1) {
                    std::cerr << filename << ": failed" << std::endl;
                }
                retval = false;
            }

            if (retval) {
                if (debuglevel > 5) {
                    std::cerr
                        << filename << ": succeeded (caught expected error)"
                        << std::endl << "error result: " << std::endl << error
                        << std::endl;

                    if (!printed_result) {
                        std::cerr
                            << "hooks result: " << std::endl << hooks
                            << std::endl;
                    }
                }
                else if (debuglevel > 4) {
                    std::cerr
                        << filename << ": succeeded (caught expected error)"
                        << std::endl << "error result: " << std::endl << error
                        << std::endl;
                }
                else if (debuglevel > 3) {
                // caught the expected error message
                    std::cerr << filename << ": succeeded" << std::endl;
                }
            }
        }
        return retval;
    }
    else {
        std::cerr
            << filename << ": no information about expected results found"
            << std::endl;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
//
//  print the current version of this program
//
///////////////////////////////////////////////////////////////////////////////
int
testwave_app::print_version()
{
// get time of last compilation of this file
boost::wave::util::time_conversion_helper compilation_time(__DATE__ " " __TIME__);

// calculate the number of days since Feb 12 2005
// (the day the testwave project was started)
std::tm first_day;

    using namespace std;      // some platforms have memset in namespace std
    memset (&first_day, 0, sizeof(std::tm));
    first_day.tm_mon = 1;           // Feb
    first_day.tm_mday = 12;         // 12
    first_day.tm_year = 105;        // 2005

long seconds = long(std::difftime(compilation_time.get_time(),
    std::mktime(&first_day)));

    std::cout
        << TESTWAVE_VERSION_MAJOR << '.'
        << TESTWAVE_VERSION_MINOR << '.'
        << TESTWAVE_VERSION_SUBMINOR << '.'
        << seconds/(3600*24)        // get number of days from seconds
        << std::endl;
    return 0;                       // exit app
}

///////////////////////////////////////////////////////////////////////////////
//
//  print the copyright statement
//
///////////////////////////////////////////////////////////////////////////////
int
testwave_app::print_copyright()
{
    char const *copyright[] = {
        "",
        "Testwave: A test driver for the Boost.Wave C++ preprocessor library",
        "http://www.boost.org/",
        "",
        "Copyright (c) 2001-2012 Hartmut Kaiser, Distributed under the Boost",
        "Software License, Version 1.0. (See accompanying file",
        "LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)",
        0
    };

    for (int i = 0; 0 != copyright[i]; ++i)
        std::cout << copyright[i] << std::endl;

    return 0;                       // exit app
}

///////////////////////////////////////////////////////////////////////////////
//
//  Read the given file into a string
//
///////////////////////////////////////////////////////////////////////////////
bool
testwave_app::read_file(std::string const& filename, std::string& instr)
{
// open the given file and report error, if appropriate
    std::ifstream instream(filename.c_str());
    if (!instream.is_open()) {
        std::cerr << "testwave: could not open input file: "
                  << filename << std::endl;
        return false;
    }
    else if (9 == debuglevel) {
        std::cerr << "read_file: succeeded to open input file: "
                  << filename << std::endl;
    }
    instream.unsetf(std::ios::skipws);

// read the input file into a string

#if defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)
// this is known to be very slow for large files on some systems
    std::copy (std::istream_iterator<char>(instream),
        std::istream_iterator<char>(),
        std::inserter(instr, instr.end()));
#else
    instr = std::string(std::istreambuf_iterator<char>(instream.rdbuf()),
        std::istreambuf_iterator<char>());
#endif

    if (9 == debuglevel) {
        std::cerr << "read_file: succeeded to read input file: "
                  << filename << std::endl;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
namespace {

    std::string const& trim_whitespace(std::string& value)
    {
        std::string::size_type first = value.find_first_not_of(" \t");
        if (std::string::npos == first)
            value.clear();
        else {
            std::string::size_type last = value.find_last_not_of(" \t");
            BOOST_ASSERT(std::string::npos != last);
            value = value.substr(first, last-first+1);
        }
        return value;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Extract special information from comments marked with the given letter
//
///////////////////////////////////////////////////////////////////////////////
bool
testwave_app::extract_special_information(std::string const& filename,
    std::string const& instr, char flag, std::string& content)
{
    if (9 == debuglevel) {
        std::cerr << "extract_special_information: extracting special information ('"
                  << flag << "') from input file: " << filename << std::endl;
    }

// tokenize the input data into C++ tokens using the C++ lexer
    typedef boost::wave::cpplexer::lex_token<> token_type;
    typedef boost::wave::cpplexer::lex_iterator<token_type> lexer_type;
    typedef token_type::position_type position_type;

    boost::wave::language_support const lang_opts =
        (boost::wave::language_support)(
            boost::wave::support_option_variadics |
            boost::wave::support_option_long_long |
            boost::wave::support_option_no_character_validation |
            boost::wave::support_option_convert_trigraphs |
            boost::wave::support_option_insert_whitespace);

    position_type pos(filename.c_str());
    lexer_type it = lexer_type(instr.begin(), instr.end(), pos, lang_opts);
    lexer_type end = lexer_type();

    try {
    // look for C or C++ comments starting with the special character
        for (/**/; it != end; ++it) {
            using namespace boost::wave;
            token_id id = token_id(*it);
            if (T_CCOMMENT == id) {
                std::string value = (*it).get_value().c_str();
                if (flag == value[2]) {
                    if (value.size() > 3 && '(' == value[3]) {
                        std::size_t p = value.find_first_of(")");
                        if (std::string::npos == p) {
                            std::cerr
                                << "testwave: missing closing parenthesis in '"
                                << flag << "()' directive" << std::endl;
                            return false;
                        }
                        std::string source = value.substr(4, p-4);
                        std::string result, error, hooks;
                        bool pp_result = preprocess_file(filename, source,
                            result, error, hooks, true);
                        if (!pp_result) {
                            std::cerr
                                << "testwave: preprocessing error in '" << flag
                                << "()' directive: " << error << std::endl;
                            return false;
                        }

                        // include this text into the extracted information
                        // only if the result is not zero
                        using namespace std;    // some system have atoi in namespace std
                        if (0 != atoi(result.c_str())) {
                            std::string thiscontent(value.substr(p+1));
                            if (9 == debuglevel) {
                                std::cerr << "extract_special_information: extracted: "
                                          << thiscontent << std::endl;
                            }
                            trim_whitespace(thiscontent);
                            content += thiscontent;
                        }
                    }
                    else {
                        std::string thiscontent(value.substr(3, value.size()-5));
                        if (9 == debuglevel) {
                            std::cerr << "extract_special_information: extracted: "
                                      << thiscontent << std::endl;
                        }
                        trim_whitespace(thiscontent);
                        content += thiscontent;
                    }
                }
            }
            else if (T_CPPCOMMENT == id) {
                std::string value = (*it).get_value().c_str();
                if (flag == value[2]) {
                    if (value.size() > 3 && '(' == value[3]) {
                        std::size_t p = value.find_first_of(")");
                        if (std::string::npos == p) {
                            std::cerr
                                << "testwave: missing closing parenthesis in '"
                                << flag << "()' directive" << std::endl;
                            return false;
                        }
                        std::string source = value.substr(4, p-4);
                        std::string result, error, hooks;
                        bool pp_result = preprocess_file(filename, source,
                            result, error, hooks, true);
                        if (!pp_result) {
                            std::cerr
                                << "testwave: preprocessing error in '" << flag
                                << "()' directive: " << error << std::endl;
                            return false;
                        }

                        // include this text into the extracted information
                        // only if the result is not zero
                        using namespace std;    // some system have atoi in namespace std
                        if (0 != atoi(result.c_str())) {
                            std::string thiscontent(value.substr((' ' == value[p+1]) ? p+2 : p+1));
                            if (9 == debuglevel) {
                                std::cerr << "extract_special_information: extracted: "
                                          << thiscontent << std::endl;
                            }
                            trim_whitespace(thiscontent);
                            content += thiscontent;
                        }
                    }
                    else {
                        std::string thiscontent(value.substr((' ' == value[3]) ? 4 : 3));
                        if (9 == debuglevel) {
                            std::cerr << "extract_special_information: extracted: "
                                      << thiscontent;
                        }
                        trim_whitespace(content);
                        content += thiscontent;
                    }
                }
            }
        }
    }
    catch (boost::wave::cpplexer::lexing_exception const &e) {
    // some lexing error
        std::cerr
            << e.file_name() << "(" << e.line_no() << "): "
            << e.description() << std::endl;
        return false;
    }

    if (9 == debuglevel) {
        std::cerr << "extract_special_information: succeeded extracting special information ('"
                  << flag << "')" << std::endl;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Extract the expected output from the given input data
//
//  The expected output has to be provided inside of special comments which
//  start with a capital 'R'. All such comments are concatenated and returned
//  through the parameter 'expected'.
//
///////////////////////////////////////////////////////////////////////////////
inline bool
testwave_app::extract_expected_output(std::string const& filename,
    std::string const& instr, std::string& expected, std::string& expectedhooks)
{
    return extract_special_information(filename, instr, 'R', expected) &&
           extract_special_information(filename, instr, 'H', expectedhooks);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Extracts the required preprocessing options from the given input data and
//  initialises the given Wave context object accordingly.
//  We allow the same (applicable) options to be used as are valid for the wave
//  driver executable.
//
///////////////////////////////////////////////////////////////////////////////
template <typename Context>
bool
testwave_app::extract_options(std::string const& filename,
    std::string const& instr, Context& ctx, bool single_line,
    po::variables_map& vm)
{
    if (9 == debuglevel) {
        std::cerr << "extract_options: extracting options" << std::endl;
    }

//  extract the required information from the comments flagged by a
//  capital 'O'
    std::string options;
    if (!extract_special_information(filename, instr, 'O', options))
        return false;

    try {
    //  parse the configuration information into a program_options_description
    //  object
        cmd_line_utils::read_config_options(debuglevel, options, desc_options, vm);
        initialise_options(ctx, vm, single_line);
    }
    catch (std::exception const &e) {
        std::cerr << filename << ": exception caught: " << e.what()
                  << std::endl;
        return false;
    }

    if (9 == debuglevel) {
        std::cerr << "extract_options: succeeded extracting options"
                  << std::endl;
    }

    return true;
}

template <typename Context>
bool
testwave_app::initialise_options(Context& ctx, po::variables_map const& vm,
    bool single_line)
{
    if (9 == debuglevel) {
        std::cerr << "initialise_options: initializing options" << std::endl;
    }

    if (vm.count("skipped_token_hooks")) {
        if (9 == debuglevel) {
            std::cerr << "initialise_options: option: skipped_token_hooks" << std::endl;
        }
        ctx.get_hooks().set_skipped_token_hooks(true);
    }

//  initialize the given context from the parsed options
#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
// enable C99 mode, if appropriate (implies variadics)
    if (vm.count("c99")) {
        if (9 == debuglevel) {
            std::cerr << "initialise_options: option: c99" << std::endl;
        }
        ctx.set_language(
            boost::wave::language_support(
                boost::wave::support_c99
              | boost::wave::support_option_emit_line_directives
#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
              | boost::wave::support_option_include_guard_detection
#endif
#if BOOST_WAVE_EMIT_PRAGMA_DIRECTIVES != 0
              | boost::wave::support_option_emit_pragma_directives
#endif
              | boost::wave::support_option_insert_whitespace
            ));
    }
    else if (vm.count("variadics")) {
    // enable variadics and placemarkers, if appropriate
        if (9 == debuglevel) {
            std::cerr << "initialise_options: option: variadics" << std::endl;
        }
        ctx.set_language(boost::wave::enable_variadics(ctx.get_language()));
    }
#endif // BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0

#if BOOST_WAVE_SUPPORT_CPP0X
    if (vm.count("c++11")) {
        if (9 == debuglevel) {
            std::cerr << "initialise_options: option: c++11" << std::endl;
        }
        ctx.set_language(
            boost::wave::language_support(
                boost::wave::support_cpp0x
              |  boost::wave::support_option_convert_trigraphs
              |  boost::wave::support_option_long_long
              |  boost::wave::support_option_emit_line_directives
#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
              |  boost::wave::support_option_include_guard_detection
#endif
#if BOOST_WAVE_EMIT_PRAGMA_DIRECTIVES != 0
              |  boost::wave::support_option_emit_pragma_directives
#endif
              |  boost::wave::support_option_insert_whitespace
            ));
    }
#endif

// enable long_long mode, if appropriate
    if (vm.count("long_long")) {
        if (9 == debuglevel) {
            std::cerr << "initialise_options: option: long_long" << std::endl;
        }
        ctx.set_language(boost::wave::enable_long_long(ctx.get_language()));
    }

// enable preserving comments mode, if appropriate
    if (vm.count("preserve")) {
        if (9 == debuglevel) {
            std::cerr << "initialise_options: option: preserve" << std::endl;
        }
        ctx.set_language(
            boost::wave::enable_preserve_comments(ctx.get_language()));
    }

// disable automatic include guard detection
    if (vm.count("noguard")) {
        if (9 == debuglevel) {
            std::cerr << "initialise_options: option: guard" << std::endl;
        }
        ctx.set_language(
            boost::wave::enable_include_guard_detection(ctx.get_language(), false));
    }

// enable trigraph conversion
    if (9 == debuglevel) {
        std::cerr << "initialise_options: option: convert_trigraphs" << std::endl;
    }
    ctx.set_language(boost::wave::enable_convert_trigraphs(ctx.get_language()));

// enable single_line mode
    if (single_line) {
        if (9 == debuglevel) {
            std::cerr << "initialise_options: option: single_line" << std::endl;
        }
        ctx.set_language(boost::wave::enable_single_line(ctx.get_language()));
        ctx.set_language(boost::wave::enable_emit_line_directives(ctx.get_language(), false));
    }

//  add include directories to the system include search paths
    if (vm.count("sysinclude")) {
    std::vector<std::string> const& syspaths =
        variables_map_as(vm["sysinclude"], (std::vector<std::string> *)NULL);

        std::vector<std::string>::const_iterator end = syspaths.end();
        for (std::vector<std::string>::const_iterator cit = syspaths.begin();
              cit != end; ++cit)
        {
            if (9 == debuglevel) {
                std::cerr << "initialise_options: option: -S" << *cit
                          << std::endl;
            }
            ctx.add_sysinclude_path((*cit).c_str());
        }
    }

//  add include directories to the user include search paths
    if (vm.count("include")) {
        cmd_line_utils::include_paths const &ip =
            variables_map_as(vm["include"], (cmd_line_utils::include_paths*)NULL);
        std::vector<std::string>::const_iterator end = ip.paths.end();

        for (std::vector<std::string>::const_iterator cit = ip.paths.begin();
              cit != end; ++cit)
        {
            if (9 == debuglevel) {
                std::cerr << "initialise_options: option: -I" << *cit
                          << std::endl;
            }
            ctx.add_include_path((*cit).c_str());
        }

    // if on the command line was given -I- , this has to be propagated
        if (ip.seen_separator) {
            if (9 == debuglevel) {
                std::cerr << "initialise_options: option: -I-" << std::endl;
            }
            ctx.set_sysinclude_delimiter();
        }

    // add system include directories to the include path
        std::vector<std::string>::const_iterator sysend = ip.syspaths.end();
        for (std::vector<std::string>::const_iterator syscit = ip.syspaths.begin();
              syscit != sysend; ++syscit)
        {
            if (9 == debuglevel) {
                std::cerr << "initialise_options: option: -S" << *syscit
                          << std::endl;
            }
            ctx.add_sysinclude_path((*syscit).c_str());
        }
    }

//  add additional defined macros
    if (vm.count("define")) {
        std::vector<std::string> const &macros =
            variables_map_as(vm["define"], (std::vector<std::string>*)NULL);
        std::vector<std::string>::const_iterator end = macros.end();
        for (std::vector<std::string>::const_iterator cit = macros.begin();
              cit != end; ++cit)
        {
            if (9 == debuglevel) {
                std::cerr << "initialise_options: option: -D" << *cit
                          << std::endl;
            }
            ctx.add_macro_definition(*cit, true);
        }
    }

//  add additional predefined macros
    if (vm.count("predefine")) {
        std::vector<std::string> const &predefmacros =
            variables_map_as(vm["predefine"], (std::vector<std::string>*)NULL);
        std::vector<std::string>::const_iterator end = predefmacros.end();
        for (std::vector<std::string>::const_iterator cit = predefmacros.begin();
              cit != end; ++cit)
        {
            if (9 == debuglevel) {
                std::cerr << "initialise_options: option: -P" << *cit
                          << std::endl;
            }
            ctx.add_macro_definition(*cit, true);
        }
    }

//  undefine specified macros
    if (vm.count("undefine")) {
        std::vector<std::string> const &undefmacros =
            variables_map_as(vm["undefine"], (std::vector<std::string>*)NULL);
        std::vector<std::string>::const_iterator end = undefmacros.end();
        for (std::vector<std::string>::const_iterator cit = undefmacros.begin();
              cit != end; ++cit)
        {
            if (9 == debuglevel) {
                std::cerr << "initialise_options: option: -U" << *cit
                          << std::endl;
            }
            ctx.remove_macro_definition(*cit);
        }
    }

//  maximal include nesting depth
    if (vm.count("nesting")) {
        int max_depth = variables_map_as(vm["nesting"], (int*)NULL);
        if (max_depth < 1 || max_depth > 100000) {
            std::cerr << "testwave: bogus maximal include nesting depth: "
                << max_depth << std::endl;
            return false;
        }
        else if (9 == debuglevel) {
            std::cerr << "initialise_options: option: -n" << max_depth
                      << std::endl;
        }
        ctx.set_max_include_nesting_depth(max_depth);
    }

    if (9 == debuglevel) {
        std::cerr << "initialise_options: succeeded to initialize options"
                  << std::endl;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//  construct a SIZEOF macro definition string and predefine this macro
template <typename Context>
inline bool
testwave_app::add_sizeof_definition(Context& ctx, char const *name, int value)
{
    BOOST_WAVETEST_OSSTREAM strm;
    strm << "__TESTWAVE_SIZEOF_" << name << "__=" << value;

    std::string macro(BOOST_WAVETEST_GETSTRING(strm));
    if (!ctx.add_macro_definition(macro, true)) {
        std::cerr << "testwave: failed to predefine macro: " << macro
                  << std::endl;
        return false;
    }
    else if (9 == debuglevel) {
        std::cerr << "add_sizeof_definition: predefined macro: " << macro
                  << std::endl;
    }
    return true;
}

//  construct a MIN macro definition string and predefine this macro
template <typename T, typename Context>
inline bool
testwave_app::add_min_definition(Context& ctx, char const *name)
{
    BOOST_WAVETEST_OSSTREAM strm;
    if (!std::numeric_limits<T>::is_signed) {
        strm << "__TESTWAVE_" << name << "_MIN__="
              << "0x" << std::hex
              << (std::numeric_limits<T>::min)() << "U";
    }
    else {
        strm << "__TESTWAVE_" << name << "_MIN__=( "
              << (std::numeric_limits<T>::min)()+1 << "-1)";
    }

    std::string macro(BOOST_WAVETEST_GETSTRING(strm));
    if (!ctx.add_macro_definition(macro, true)) {
        std::cerr << "testwave: failed to predefine macro: " << macro
                  << std::endl;
        return false;
    }
    else if (9 == debuglevel) {
        std::cerr << "add_min_definition: predefined macro: " << macro
                  << std::endl;
    }
    return true;
}

//  construct a MAX macro definition string and predefine this macro
template <typename T, typename Context>
inline bool
testwave_app::add_max_definition(Context& ctx, char const *name)
{
    BOOST_WAVETEST_OSSTREAM strm;
    if (!std::numeric_limits<T>::is_signed) {
        strm << "__TESTWAVE_" << name << "_MAX__="
              << "0x" << std::hex
              << (std::numeric_limits<T>::max)() << "U";
    }
    else {
        strm << "__TESTWAVE_" << name << "_MAX__="
              << (std::numeric_limits<T>::max)();
    }

    std::string macro(BOOST_WAVETEST_GETSTRING(strm));
    if (!ctx.add_macro_definition(macro, true)) {
        std::cerr << "testwave: failed to predefine macro: " << macro
                  << std::endl;
        return false;
    }
    else if (9 == debuglevel) {
        std::cerr << "add_max_definition: predefined macro: " << macro
                  << std::endl;
    }
    return true;
}

//  Predefine __TESTWAVE_HAS_STRICT_LEXER__
template <typename Context>
inline bool
testwave_app::add_strict_lexer_definition(Context& ctx)
{
    std::string macro("__TESTWAVE_HAS_STRICT_LEXER__=1");
    if (!ctx.add_macro_definition(macro, true)) {
        std::cerr << "testwave: failed to predefine macro: " << macro
                  << std::endl;
        return false;
    }
    else if (9 == debuglevel) {
        std::cerr << "add_strict_lexer_definition: predefined macro: " << macro
                  << std::endl;
    }
    return true;
}

#if BOOST_WAVE_SUPPORT_MS_EXTENSIONS
//  Predefine __TESTWAVE_SUPPORT_MS_EXTENSIONS__
template <typename Context>
inline bool
testwave_app::add_support_ms_extensions_definition(Context& ctx)
{
    std::string macro("__TESTWAVE_SUPPORT_MS_EXTENSIONS__=1");
    if (!ctx.add_macro_definition(macro, true)) {
        std::cerr << "testwave: failed to predefine macro: " << macro
                  << std::endl;
        return false;
    }
    else if (9 == debuglevel) {
        std::cerr << "add_support_ms_extensions_definition: predefined macro: " 
                  << macro 
                  << std::endl;
    }
    return true;
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Add special predefined macros to the context object.
//
//  This adds a lot of macros to the test environment, which allows to adjust
//  the test cases for different platforms.
//
///////////////////////////////////////////////////////////////////////////////
template <typename Context>
bool
testwave_app::add_predefined_macros(Context& ctx)
{
    // add the __TESTWAVE_SIZEOF_<type>__ macros
    if (!add_sizeof_definition(ctx, "CHAR", sizeof(char)) ||
        !add_sizeof_definition(ctx, "SHORT", sizeof(short)) ||
        !add_sizeof_definition(ctx, "INT", sizeof(int)) ||
#if defined(BOOST_HAS_LONG_LONG)
        !add_sizeof_definition(ctx, "LONGLONG", sizeof(boost::long_long_type)) ||
#endif
        !add_sizeof_definition(ctx, "LONG", sizeof(long)))
    {
        std::cerr << "testwave: failed to add a predefined macro (SIZEOF)."
                  << std::endl;
        return false;
    }

    // add the __TESTWAVE_<type>_MIN__ macros
    if (/*!add_min_definition<char>(ctx, "CHAR") ||*/
        /*!add_min_definition<unsigned char>(ctx, "UCHAR") ||*/
        !add_min_definition<short>(ctx, "SHORT") ||
        !add_min_definition<unsigned short>(ctx, "USHORT") ||
        !add_min_definition<int>(ctx, "INT") ||
        !add_min_definition<unsigned int>(ctx, "UINT") ||
#if defined(BOOST_HAS_LONG_LONG)
        !add_min_definition<boost::long_long_type>(ctx, "LONGLONG") ||
        !add_min_definition<boost::ulong_long_type>(ctx, "ULONGLONG") ||
#endif
        !add_min_definition<long>(ctx, "LONG") ||
        !add_min_definition<unsigned long>(ctx, "ULONG"))
    {
        std::cerr << "testwave: failed to add a predefined macro (MIN)."
                  << std::endl;
    }

    // add the __TESTWAVE_<type>_MAX__ macros
    if (/*!add_max_definition<char>(ctx, "CHAR") ||*/
        /*!add_max_definition<unsigned char>(ctx, "UCHAR") ||*/
        !add_max_definition<short>(ctx, "SHORT") ||
        !add_max_definition<unsigned short>(ctx, "USHORT") ||
        !add_max_definition<int>(ctx, "INT") ||
        !add_max_definition<unsigned int>(ctx, "UINT") ||
#if defined(BOOST_HAS_LONG_LONG)
        !add_max_definition<boost::long_long_type>(ctx, "LONGLONG") ||
        !add_max_definition<boost::ulong_long_type>(ctx, "ULONGLONG") ||
#endif
        !add_max_definition<long>(ctx, "LONG") ||
        !add_max_definition<unsigned long>(ctx, "ULONG"))
    {
        std::cerr << "testwave: failed to add a predefined macro (MAX)."
                  << std::endl;
    }

#if BOOST_WAVE_SUPPORT_MS_EXTENSIONS
//  Predefine __TESTWAVE_SUPPORT_MS_EXTENSIONS__
    if (!add_support_ms_extensions_definition(ctx))
    {
        std::cerr << "testwave: failed to add a predefined macro "
                     "(__TESTWAVE_SUPPORT_MS_EXTENSIONS__)."
                  << std::endl;
    }
#endif

#if BOOST_WAVE_USE_STRICT_LEXER != 0
    return add_strict_lexer_definition(ctx);
#else
    return true;
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
//  Preprocess the given input data and return the generated output through
//  the parameter 'result'.
//
///////////////////////////////////////////////////////////////////////////////
bool
testwave_app::preprocess_file(std::string filename, std::string const& instr,
    std::string& result, std::string& error, std::string& hooks,
    bool single_line)
{
//  create the wave::context object and initialize it from the file to
//  preprocess (may contain options inside of special comments)
    typedef boost::wave::cpplexer::lex_token<> token_type;
    typedef boost::wave::cpplexer::lex_iterator<token_type> lexer_type;
    typedef boost::wave::context<
        std::string::const_iterator, lexer_type,
        boost::wave::iteration_context_policies::load_file_to_string,
        collect_hooks_information<token_type> >
    context_type;

    if (9 == debuglevel) {
        std::cerr << "preprocess_file: preprocessing input file: " << filename
                  << std::endl;
    }

    try {
    //  create preprocessing context
        context_type ctx(instr.begin(), instr.end(), filename.c_str(),
            collect_hooks_information<token_type>(hooks));

    //  initialize the context from the options given on the command line
        if (!initialise_options(ctx, global_vm, single_line))
            return false;

    //  extract the options from the input data and initialize the context
        boost::program_options::variables_map local_vm;
        if (!extract_options(filename, instr, ctx, single_line, local_vm))
            return false;

    //  add special predefined macros
        if (!add_predefined_macros(ctx))
            return false;

    //  preprocess the input, loop over all generated tokens collecting the
    //  generated text
        context_type::iterator_type it = ctx.begin();
        context_type::iterator_type end = ctx.end();

        if (local_vm.count("forceinclude")) {
        // add the filenames to force as include files in _reverse_ order
        // the second parameter 'is_last' of the force_include function should
        // be set to true for the last (first given) file.
            std::vector<std::string> const &force =
                local_vm["forceinclude"].as<std::vector<std::string> >();
            std::vector<std::string>::const_reverse_iterator rend = force.rend();
            for (std::vector<std::string>::const_reverse_iterator cit = force.rbegin();
                 cit != rend; /**/)
            {
                std::string forceinclude(*cit);
                if (9 == debuglevel) {
                    std::cerr << "preprocess_file: option: forceinclude ("
                              << forceinclude << ")" << std::endl;
                }
                it.force_include(forceinclude.c_str(), ++cit == rend);
            }
        }

    // perform actual preprocessing
        for (/**/; it != end; ++it)
        {
            using namespace boost::wave;

            if (T_PP_LINE == token_id(*it)) {
            // special handling of the whole #line directive is required to
            // allow correct file name matching
                if (!handle_line_directive(it, end, result))
                    return false;   // unexpected eof
            }
            else {
                // add the value of the current token
                result = result + (*it).get_value().c_str();
            }
        }
        error.clear();
    }
    catch (boost::wave::cpplexer::lexing_exception const& e) {
    // some lexer error
        BOOST_WAVETEST_OSSTREAM strm;
        std::string filename = e.file_name();
        strm
            << handle_filepath(filename) << "(" << e.line_no() << "): "
            << e.description() << std::endl;

        error = BOOST_WAVETEST_GETSTRING(strm);
        return false;
    }
    catch (boost::wave::cpp_exception const& e) {
    // some preprocessing error
        BOOST_WAVETEST_OSSTREAM strm;
        std::string filename = e.file_name();
        strm
            << handle_filepath(filename) << "(" << e.line_no() << "): "
            << e.description() << std::endl;

        error = BOOST_WAVETEST_GETSTRING(strm);
        return false;
    }

    if (9 == debuglevel) {
        std::cerr << "preprocess_file: succeeded to preprocess input file: "
                  << filename << std::endl;
    }

    return true;
}

