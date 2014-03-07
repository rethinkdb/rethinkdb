/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2013 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_WAVE_LIBS_WAVE_TEST_TESTWAVE_APP_HPP)
#define BOOST_WAVE_LIBS_WAVE_TEST_TESTWAVE_APP_HPP

#include <string>
#include <vector>

// include boost
#include <boost/config.hpp>

#include "cmd_line_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
class testwave_app
{
public:
    testwave_app(boost::program_options::variables_map const& vm);

    //  Test the given file (i.e. preprocess the file and compare the result
    //  against the embedded 'R' comments, if an error occurs compare the error
    //  message against the given 'E' comments).
    bool test_a_file(std::string filename);

    //  print the current version of this program
    int print_version();

    //  print the copyright statement
    int print_copyright();

    // access the common options used for the command line and the config
    // options inside the test files
    boost::program_options::options_description const& common_options() const
    {
        return desc_options;
    }

    void set_debuglevel(int debuglevel_)
    {
        debuglevel = debuglevel_;
    }
    int get_debuglevel() const
    {
        return debuglevel;
    }

protected:
    //  Read the given file into a string
    bool read_file(std::string const& filename, std::string& instr);

    //  Extract special information from comments marked with the given letter
    bool extract_special_information(std::string const& filename,
        std::string const& instr, char flag, std::string& content);

    //  Extract the expected output and expected hooks information from the
    //  given input data.
    //  The expected output has to be provided inside of special comments which
    //  start with a capital 'R' ('H' for the hooks information). All such
    //  comments are concatenated and returned through the parameter 'expected'
    //  ('expectedhooks' for hooks information).
    bool extract_expected_output(std::string const& filename,
        std::string const& instr, std::string& expected,
        std::string& expectedhooks);

    //  Extracts the required preprocessing options from the given input data
    //  and initializes the given Wave context object accordingly.
    //  We allow the same (applicable) options to be used as are valid for the
    //  wave driver executable.
    template <typename Context>
    bool extract_options(std::string const& filename,
        std::string const& instr, Context& ctx, bool single_line,
        boost::program_options::variables_map& vm);

    //  transfers the options collected in the vm parameter into the given
    //  context
    template <typename Context>
    bool initialise_options(Context& ctx,
        boost::program_options::variables_map const& vm, bool single_line);

    //  Preprocess the given input data and return the generated output through
    //  the parameter 'result'.
    bool preprocess_file(std::string filename, std::string const& instr,
        std::string& result, std::string& error, std::string& hooks,
        bool single_line = false);

    //  Add special predefined macros to the context object
    template <typename Context>
    bool add_predefined_macros(Context& ctx);

    //  This function compares the real result and the expected one but first
    //  replaces all occurrences in the expected result of
    //      $E: to the result of preprocessing the given expression
    //      $F: to the passed full filepath
    //      $P: to the full path
    //      $R: to the relative path
    //      $V: to the current Boost version number
    bool got_expected_result(std::string const& filename,
        std::string const& result, std::string& expected);

    //  construct a SIZEOF macro definition string and predefine this macro
    template <typename Context>
    bool add_sizeof_definition(Context& ctx, char const *name, int value);

    //  construct a MIN macro definition string and predefine this macro
    template <typename T, typename Context>
    bool add_min_definition(Context& ctx, char const *name);

    //  construct a MAX macro definition string and predefine this macro
    template <typename T, typename Context>
    bool add_max_definition(Context& ctx, char const *name);

    //  Predefine __TESTWAVE_HAS_STRICT_LEXER__
    template <typename Context>
    bool add_strict_lexer_definition(Context& ctx);

#if BOOST_WAVE_SUPPORT_MS_EXTENSIONS
    //  Predefine __TESTWAVE_SUPPORT_MS_EXTENSIONS__
    template <typename Context>
    bool add_support_ms_extensions_definition(Context& ctx);
#endif

private:
    int debuglevel;
    boost::program_options::options_description desc_options;
    boost::program_options::variables_map const& global_vm;
};

#endif // !defined(BOOST_WAVE_LIBS_WAVE_TEST_TESTWAVE_APP_HPP)
