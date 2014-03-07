// Copyright Leo Goodstadt 2012
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <boost/program_options/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/cmdline.hpp>
using namespace boost::program_options;

#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>
using namespace std;

#include "minitest.hpp"



// 
//  like BOOST_CHECK_EQUAL but with more descriptive error message
//
#define CHECK_EQUAL(description, a, b) if (a != b) {std::cerr << "\n\nError:\n<<" << \
        description << ">>\n  Expected text=\"" << b << "\"\n  Actual text  =\"" << a << "\"\n\n"; assert(a == b);}


// Uncomment for Debugging, removes asserts so we can see more failures!
//#define BOOST_ERROR(description) std::cerr << description; std::cerr << "\n"; 


//8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888
// 
//  Uncomment to print out the complete set of diagnostic messages for the different test cases
/*
#define CHECK_EQUAL(description, a, b) if (a != b) {std::cerr << "\n\nError:  " << \
    description << "\n  Expecting\n" << b << "\n  Found\n" << a << "\n\n"; } \
    else {std::cout << description<< "\t" << b << "\n";}
*/ 

//8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888


// 
//  test exception for each specified command line style, e.g. short dash or config file
//
template<typename EXCEPTION>
void test_each_exception_message(const string& test_description, const vector<const char*>& argv, options_description& desc, int style, string exception_msg, istream& is = cin)
{
    if (exception_msg.length() == 0)
        return;
    variables_map vm;
    unsigned argc = argv.size();


    try {
        if (style == -1)
            store(parse_config_file(is, desc), vm);
        else
            store(parse_command_line(argv.size(), argv.data(), desc, style), vm);
        notify(vm);    
    } 
    catch (EXCEPTION& e)
    {
       //cerr << "Correct:\n\t" << e.what() << "\n";
       CHECK_EQUAL(test_description, e.what(), exception_msg);
       return;
    }
    catch (std::exception& e)
    {
        // concatenate argv without boost::algorithm::join
        string argv_txt;
        for (unsigned ii = 0; ii < argc - 1; ++ii)
            argv_txt += argv[ii] + string(" ");
        if (argc)
            argv_txt += argv[argc - 1];

       BOOST_ERROR("\n<<" + test_description + 
                    string(">>\n  Unexpected exception type!\n  Actual text  =\"") + e.what() + 
                    "\"\n  argv         =\"" + argv_txt +
                    "\"\n  Expected text=\"" + exception_msg + "\"\n");
       return;
    }
    BOOST_ERROR(test_description + ": No exception thrown. ");
}




// 
//  test exception messages for all command line styles (unix/long/short/slash/config file)
//
//      try each command line style in turn
const int unix_style = command_line_style::unix_style;
const int short_dash = command_line_style::allow_dash_for_short | command_line_style::allow_short | command_line_style::short_allow_adjacent | command_line_style::allow_sticky;
const int short_slash = command_line_style::allow_slash_for_short | command_line_style::allow_short | command_line_style::short_allow_adjacent;
const int long_dash = command_line_style::allow_long | command_line_style::long_allow_adjacent | command_line_style::allow_guessing;



template<typename EXCEPTION>
void test_exception_message(const vector<vector<const char*>>& argv,
                            options_description& desc,
                            const string& error_description, 
                            const char* expected_message_template[5])
{
    string expected_message;

    // unix
    expected_message = expected_message_template[0];
    test_each_exception_message<EXCEPTION>(error_description + " -- unix", 
                                  argv[0], desc, unix_style, expected_message);

    // long dash only
    expected_message = expected_message_template[1];
    test_each_exception_message<EXCEPTION>(error_description + " -- long_dash", 
                                  argv[1], desc, long_dash, expected_message);


    // short dash only
    expected_message = expected_message_template[2];
    test_each_exception_message<EXCEPTION>(error_description + " -- short_dash", 
                                  argv[2], desc, short_dash, expected_message);

    // short slash only
    expected_message = expected_message_template[3];
    test_each_exception_message<EXCEPTION>(error_description + " -- short_slash", 
                                  argv[3], desc, short_slash, expected_message);

    // config file only
    expected_message = expected_message_template[4];
    if (expected_message.length())
    {
        istringstream istrm(argv[4][0]);
        test_each_exception_message<EXCEPTION>(error_description + " -- config_file", 
                                          argv[4], desc, -1, expected_message, istrm);
    }

}

#define VEC_STR_PUSH_BACK(vec, c_array)  \
    vec.push_back(vector<const char*>(c_array, c_array + sizeof(c_array) / sizeof(char*)));

//________________________________________________________________________________________
// 
//  invalid_option_value
//
//________________________________________________________________________________________
void test_invalid_option_value_exception_msg()
{
    options_description desc;
    desc.add_options()
        ("int_option,d", value< int >(),	"An option taking an integer")
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--int", "A_STRING"};  VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "int_option=A_STRING"}         ;  VEC_STR_PUSH_BACK(argv, argv4); 

    const char* expected_msg[5] = {
                                "the argument ('A_STRING') for option '--int_option' is invalid", 
                                "the argument ('A_STRING') for option '--int_option' is invalid", 
                                "the argument ('A_STRING') for option '-d' is invalid", 
                                "the argument ('A_STRING') for option '/d' is invalid", 
                                "the argument ('A_STRING') for option 'int_option' is invalid"
    };


    test_exception_message<invalid_option_value>(argv, desc, "invalid_option_value",
                                                 expected_msg);


}

//________________________________________________________________________________________
// 
//  missing_value
//
//________________________________________________________________________________________
void test_missing_value_exception_msg()
{
    options_description desc;
    desc.add_options()
        ("cfgfile,e", value<string>(), "the config file") 
        ("output,o", value<string>(), "the output file")
    ;
    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-e", "-e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfgfile"}              ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-e", "-e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/e", "/e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { ""}    ;      VEC_STR_PUSH_BACK(argv, argv4); 

    const char* expected_msg[5] = {
                                "the required argument for option '--cfgfile' is missing",
                                "the required argument for option '--cfgfile' is missing",
                                "the required argument for option '-e' is missing",
                                "", // Ignore probable bug in cmdline::finish_option
                                    //"the required argument for option '/e' is missing",
                                "",
    };
    test_exception_message<invalid_command_line_syntax>(argv, desc, 
                                                        "invalid_syntax::missing_parameter", 
                                                        expected_msg);
}

//________________________________________________________________________________________
// 
//  ambiguous_option
//
//________________________________________________________________________________________
void test_ambiguous_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("cfgfile1,c", value<string>(), "the config file") 
        ("cfgfile2,o",  value<string>(), "the config file") 
        ("good,g",                          "good option") 
        ("output,c",   value<string>(), "the output file") 
        ("output",     value<string>(), "the output file") 
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-ggc", "file", "-o", "anotherfile"}             ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfile", "file", "--cfgfile", "anotherfile"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-ggc", "file", "-o", "anotherfile"}             ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file", "/o", "anotherfile"}               ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "output=output.txt\n"}                                     ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "option '-c' is ambiguous and matches '--cfgfile1', and '--output'",
                                "option '--cfgfile' is ambiguous and matches '--cfgfile1', and '--cfgfile2'",
                                "option '-c' is ambiguous",
                                "option '/c' is ambiguous",
                                "option 'output' is ambiguous and matches different versions of 'output'",
    };
    test_exception_message<ambiguous_option>(argv, desc, "ambiguous_option",
                                             expected_msg);
}

//________________________________________________________________________________________
// 
//  multiple_occurrences
//
//________________________________________________________________________________________
void test_multiple_occurrences_exception_msg()
{
    options_description desc; 
    desc.add_options()
                ("cfgfile,c", value<string>(), "the configfile")
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-c", "file", "-c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfi", "file", "--cfgfi", "anotherfile"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-c", "file", "-c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file", "/c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "cfgfile=output.txt\ncfgfile=output.txt\n"}            ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "option '--cfgfile' cannot be specified more than once",
                                "option '--cfgfile' cannot be specified more than once",
                                "option '-c' cannot be specified more than once",
                                "option '/c' cannot be specified more than once",
                                "option 'cfgfile' cannot be specified more than once",
    };
    test_exception_message<multiple_occurrences>(argv, desc, "multiple_occurrences",
                                                expected_msg);
}

//________________________________________________________________________________________
// 
//  unknown_option
//
//________________________________________________________________________________________
void test_unknown_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("good,g",                          "good option") 
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-ggc", "file"}      ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfile", "file"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-ggc", "file"}      ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file"}        ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "cfgfile=output.txt\n"}        ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "unrecognised option '-ggc'",
                                "unrecognised option '--cfgfile'",
                                "unrecognised option '-ggc'",
                                "unrecognised option '/c'",
                                "unrecognised option 'cfgfile'",
    };
    test_exception_message<unknown_option>(argv, desc, "unknown_option", expected_msg);
}

//________________________________________________________________________________________
// 
//  validation_error::invalid_bool_value
//
//________________________________________________________________________________________
void test_invalid_bool_value_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("bool_option,b",	value< bool>(),	"bool_option")
        ;


    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--bool_optio", "file"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "bool_option=output.txt\n"}       ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "the argument ('file') for option '--bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '--bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '-b' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '/b' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('output.txt') for option 'bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::invalid_bool_value",
                                expected_msg);
}




//________________________________________________________________________________________
// 
//  validation_error::multiple_values_not_allowed
//
//________________________________________________________________________________________
//
//  Strange exception: sole purpose seems to be catching multitoken() associated with a scalar
//  validation_error::multiple_values_not_allowed seems thus to be a programmer error
//  
//
void test_multiple_values_not_allowed_exception_msg()
{
    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<string>()->multitoken(), "the config file")
         ("good,g",                                     "good option")
         ("output,o", value<string>(),                  "the output file")
       ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-c", "file", "c", "-o", "fritz", "hugo" }              ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfgfil", "file", "c", "--outpu", "fritz", "hugo" }   ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-c", "file", "c", "-o", "fritz", "hugo"}               ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/c", "file", "c", "/o", "fritz", "hugo"}               ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "" }                                                               ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "option '--cfgfile' only takes a single argument",
                                "option '--cfgfile' only takes a single argument",
                                "option '-c' only takes a single argument",
                                "option '/c' only takes a single argument",
                                "",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::multiple_values_not_allowed",
                                expected_msg);
}

//________________________________________________________________________________________
// 
//  validation_error::at_least_one_value_required
//
//________________________________________________________________________________________
//
//  Strange exception: sole purpose seems to be catching zero_tokens() associated with a scalar
//  validation_error::multiple_values_not_allowed seems thus to be a programmer error
//  
//
void test_at_least_one_value_required_exception_msg()
{


    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<int>()->zero_tokens(), "the config file")
         ("other,o", value<string>(), "other")
       ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-c"                       }  ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfg", "--o", "name"     }  ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-c"   , "-o"   , "name"   }  ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/c"                       }  ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { ""                                    }  ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "option '--cfgfile' requires at least one argument",
                                "option '--cfgfile' requires at least one argument",
                                "option '-c' requires at least one argument",
                                "option '/c' requires at least one argument",
                                "",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::at_least_one_value_required",
                                expected_msg);
}


//________________________________________________________________________________________
// 
//  required_option
//
//________________________________________________________________________________________
void test_required_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<string>()->required(), "the config file")
         ("good,g",                                 "good option")
         ("output,o", value<string>()->required(),  "the output file")
       ;

    vector<vector<const char*>> argv;            
    const char* argv0[] = { "program", "-g" }    ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--g" }   ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-g"}     ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/g"}     ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "" }                 ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "the option '--cfgfile' is required but missing",
                                "the option '--cfgfile' is required but missing",
                                "the option '-c' is required but missing",
                                "the option '/c' is required but missing",
                                "the option 'cfgfile' is required but missing",
    };
    test_exception_message<required_option>(argv, 
                                desc,
                                "required_option",
                                expected_msg);
}
















/** 
 * Check if this is the expected exception with the right message is being thrown inside 
 * func 
*/
template <typename EXCEPTION, typename FUNC>
void test_exception(const string& test_name, const string& exception_txt, FUNC func)
{

    try {
        options_description desc; 
        variables_map vm;
        func(desc, vm);
    } 
    catch (EXCEPTION& e)
    {
       CHECK_EQUAL(test_name, e.what(), exception_txt);
       return;
    }
    catch (std::exception& e)
    {
       BOOST_ERROR(string(test_name + ":\nUnexpected exception. ") + e.what() + 
                   "\nExpected text:\n" + exception_txt + "\n\n");
       return;
    }
    BOOST_ERROR(test_name + ": No exception thrown. ");
}



//________________________________________________________________________________________
// 
//  check_reading_file
//
//________________________________________________________________________________________
void check_reading_file(options_description& desc, variables_map& vm)
{
    desc.add_options()
         ("output,o", value<string>(), "the output file");
    
    const char* file_name = "no_such_file";
    store(parse_config_file<char>(file_name, desc, true), vm);

}


//________________________________________________________________________________________
// 
//  config_file_wildcard
//
//________________________________________________________________________________________
void config_file_wildcard(options_description& desc, variables_map& vm)
{
    desc.add_options()
         ("outpu*", value<string>(), "the output file1")
         ("outp*",    value<string>(), "the output file2")
       ;
    istringstream is("output1=whichone\noutput2=whichone\n");
    store(parse_config_file(is, desc), vm);
}

//________________________________________________________________________________________
// 
//  invalid_syntax::unrecognized_line
//
//________________________________________________________________________________________
void unrecognized_line(options_description& desc, variables_map& vm)
{
    istringstream is("funny wierd line\n");
    store(parse_config_file(is, desc), vm);
}

//________________________________________________________________________________________
// 
//  abbreviated_options_in_config_file
//
//________________________________________________________________________________________
void abbreviated_options_in_config_file(options_description& desc, variables_map& vm)
{
    desc.add_options()(",o", value<string>(), "the output file");
    istringstream is("o=output.txt\n");
    store(parse_config_file(is, desc), vm);
}


//________________________________________________________________________________________
// 
//  too_many_positional_options
//
//________________________________________________________________________________________
void too_many_positional_options(options_description& desc, variables_map& vm)
{
    const char* argv[] = {"program", "1", "2", "3"};
    positional_options_description positional_args;
    positional_args.add("two_positional_arguments", 2);
    store(command_line_parser(4, argv).options(desc).positional(positional_args).run(), vm);
}


//________________________________________________________________________________________
// 
//  invalid_command_line_style
//
//________________________________________________________________________________________

void test_invalid_command_line_style_exception_msg()
{
    string test_name = "invalid_command_line_style";
    using namespace command_line_style;
    options_description desc; 
    desc.add_options()("output,o", value<string>(), "the output file");

    vector<int> invalid_styles;
    invalid_styles.push_back(allow_short | short_allow_adjacent);
    invalid_styles.push_back(allow_short | allow_dash_for_short);
    invalid_styles.push_back(allow_long);
    vector<string> invalid_diagnostics;
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of 'command_line_style::allow_slash_for_short' "
                                  "(slashes) or 'command_line_style::allow_dash_for_short' "
                                  "(dashes) for short options.");
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of 'command_line_style::short_allow_next' "
                                  "(whitespace separated arguments) or "
                                  "'command_line_style::short_allow_adjacent' ('=' "
                                  "separated arguments) for short options.");
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of 'command_line_style::long_allow_next' "
                                  "(whitespace separated arguments) or "
                                  "'command_line_style::long_allow_adjacent' ('=' "
                                  "separated arguments) for long options.");


    const char* argv[] = {"program"};
    variables_map vm;
    for (unsigned ii = 0; ii < 3; ++ii)
    {
        bool exception_thrown = false;
        try
        {
            store(parse_command_line(1, argv, desc, invalid_styles[ii]), vm);
        }
        catch (invalid_command_line_style& e)
        {
           string error_msg("arguments are not allowed for unabbreviated option names");
           CHECK_EQUAL(test_name, e.what(), invalid_diagnostics[ii]);
           exception_thrown = true;
        }
        catch (std::exception& e)
        {
           BOOST_ERROR(string(test_name + ":\nUnexpected exception. ") + e.what() + 
                            "\nExpected text:\n" + invalid_diagnostics[ii] + "\n");
           exception_thrown = true;
        }
        if (!exception_thrown)
        {
            BOOST_ERROR(test_name << ": No exception thrown. ");
        }
    }
}



int main(int /*ac*/, char** /*av*/)
{
   test_ambiguous_option_exception_msg();
   test_unknown_option_exception_msg();
   test_multiple_occurrences_exception_msg();
   test_missing_value_exception_msg();
   test_invalid_option_value_exception_msg();
   test_invalid_bool_value_exception_msg();
   test_multiple_values_not_allowed_exception_msg();
   test_required_option_exception_msg();
   test_at_least_one_value_required_exception_msg();

   string test_name;
   string expected_message;


   // check_reading_file
   test_name        = "check_reading_file";
   expected_message = "can not read options configuration file 'no_such_file'";
   test_exception<reading_file>(test_name, expected_message, check_reading_file);

   // config_file_wildcard
   test_name        = "config_file_wildcard";
   expected_message = "options 'outpu*' and 'outp*' will both match the same arguments from the configuration file";
   test_exception<error>(test_name, expected_message, config_file_wildcard);

   // unrecognized_line
   test_name        = "unrecognized_line";
   expected_message = "the options configuration file contains an invalid line 'funny wierd line'";
   test_exception<invalid_syntax>(test_name, expected_message, unrecognized_line);


   // abbreviated_options_in_config_file
   test_name        = "abbreviated_options_in_config_file";
   expected_message = "abbreviated option names are not permitted in options configuration files";
   test_exception<error>(test_name, expected_message, abbreviated_options_in_config_file);

   test_name        = "too_many_positional_options";
   expected_message = "too many positional arguments have been specified on the command line";
   test_exception<too_many_positional_options_error>(
                          test_name, expected_message, too_many_positional_options);

   test_invalid_command_line_style_exception_msg();


   return 0;
}



// Copyright Sascha Ochsenknecht 2009.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <boost/program_options/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/cmdline.hpp>
using namespace boost::program_options;

#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>
using namespace std;

#include "minitest.hpp"



// 
//  like BOOST_CHECK_EQUAL but with more descriptive error message
//
#define CHECK_EQUAL(description, a, b) if (a != b) {std::cerr << "\n\nError:\n<<" << \
        description << ">>\n  Expected text=\"" << b << "\"\n  Actual text  =\"" << a << "\"\n\n"; assert(a == b);}


// Uncomment for Debugging, removes asserts so we can see more failures!
//#define BOOST_ERROR(description) std::cerr << description; std::cerr << "\n"; 


//8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888
// 
//  Uncomment to print out the complete set of diagnostic messages for the different test cases
/*
#define CHECK_EQUAL(description, a, b) if (a != b) {std::cerr << "\n\nError:  " << \
    description << "\n  Expecting\n" << b << "\n  Found\n" << a << "\n\n"; } \
    else {std::cout << description<< "\t" << b << "\n";}
*/ 

//8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888


// 
//  test exception for each specified command line style, e.g. short dash or config file
//
template<typename EXCEPTION>
void test_each_exception_message(const string& test_description, const vector<const char*>& argv, options_description& desc, int style, string exception_msg, istream& is = cin)
{
    if (exception_msg.length() == 0)
        return;
    variables_map vm;
    unsigned argc = argv.size();


    try {
        if (style == -1)
            store(parse_config_file(is, desc), vm);
        else
            store(parse_command_line(argv.size(), argv.data(), desc, style), vm);
        notify(vm);    
    } 
    catch (EXCEPTION& e)
    {
       //cerr << "Correct:\n\t" << e.what() << "\n";
       CHECK_EQUAL(test_description, e.what(), exception_msg);
       return;
    }
    catch (std::exception& e)
    {
        // concatenate argv without boost::algorithm::join
        string argv_txt;
        for (unsigned ii = 0; ii < argc - 1; ++ii)
            argv_txt += argv[ii] + string(" ");
        if (argc)
            argv_txt += argv[argc - 1];

       BOOST_ERROR("\n<<" + test_description + 
                    string(">>\n  Unexpected exception type!\n  Actual text  =\"") + e.what() + 
                    "\"\n  argv         =\"" + argv_txt +
                    "\"\n  Expected text=\"" + exception_msg + "\"\n");
       return;
    }
    BOOST_ERROR(test_description + ": No exception thrown. ");
}




// 
//  test exception messages for all command line styles (unix/long/short/slash/config file)
//
//      try each command line style in turn
const int unix_style = command_line_style::unix_style;
const int short_dash = command_line_style::allow_dash_for_short | command_line_style::allow_short | command_line_style::short_allow_adjacent | command_line_style::allow_sticky;
const int short_slash = command_line_style::allow_slash_for_short | command_line_style::allow_short | command_line_style::short_allow_adjacent;
const int long_dash = command_line_style::allow_long | command_line_style::long_allow_adjacent | command_line_style::allow_guessing;



template<typename EXCEPTION>
void test_exception_message(const vector<vector<const char*>>& argv,
                            options_description& desc,
                            const string& error_description, 
                            const char* expected_message_template[5])
{
    string expected_message;

    // unix
    expected_message = expected_message_template[0];
    test_each_exception_message<EXCEPTION>(error_description + " -- unix", 
                                  argv[0], desc, unix_style, expected_message);

    // long dash only
    expected_message = expected_message_template[1];
    test_each_exception_message<EXCEPTION>(error_description + " -- long_dash", 
                                  argv[1], desc, long_dash, expected_message);


    // short dash only
    expected_message = expected_message_template[2];
    test_each_exception_message<EXCEPTION>(error_description + " -- short_dash", 
                                  argv[2], desc, short_dash, expected_message);

    // short slash only
    expected_message = expected_message_template[3];
    test_each_exception_message<EXCEPTION>(error_description + " -- short_slash", 
                                  argv[3], desc, short_slash, expected_message);

    // config file only
    expected_message = expected_message_template[4];
    if (expected_message.length())
    {
        istringstream istrm(argv[4][0]);
        test_each_exception_message<EXCEPTION>(error_description + " -- config_file", 
                                          argv[4], desc, -1, expected_message, istrm);
    }

}

#define VEC_STR_PUSH_BACK(vec, c_array)  \
    vec.push_back(vector<const char*>(c_array, c_array + sizeof(c_array) / sizeof(char*)));

//________________________________________________________________________________________
// 
//  invalid_option_value
//
//________________________________________________________________________________________
void test_invalid_option_value_exception_msg()
{
    options_description desc;
    desc.add_options()
        ("int_option,d", value< int >(),	"An option taking an integer")
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--int", "A_STRING"};  VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "int_option=A_STRING"}         ;  VEC_STR_PUSH_BACK(argv, argv4); 

    const char* expected_msg[5] = {
                                "the argument ('A_STRING') for option '--int_option' is invalid", 
                                "the argument ('A_STRING') for option '--int_option' is invalid", 
                                "the argument ('A_STRING') for option '-d' is invalid", 
                                "the argument ('A_STRING') for option '/d' is invalid", 
                                "the argument ('A_STRING') for option 'int_option' is invalid"
    };


    test_exception_message<invalid_option_value>(argv, desc, "invalid_option_value",
                                                 expected_msg);


}

//________________________________________________________________________________________
// 
//  missing_value
//
//________________________________________________________________________________________
void test_missing_value_exception_msg()
{
    options_description desc;
    desc.add_options()
        ("cfgfile,e", value<string>(), "the config file") 
        ("output,o", value<string>(), "the output file")
    ;
    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-e", "-e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfgfile"}              ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-e", "-e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/e", "/e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { ""}    ;      VEC_STR_PUSH_BACK(argv, argv4); 

    const char* expected_msg[5] = {
                                "the required argument for option '--cfgfile' is missing",
                                "the required argument for option '--cfgfile' is missing",
                                "the required argument for option '-e' is missing",
                                "", // Ignore probable bug in cmdline::finish_option
                                    //"the required argument for option '/e' is missing",
                                "",
    };
    test_exception_message<invalid_command_line_syntax>(argv, desc, 
                                                        "invalid_syntax::missing_parameter", 
                                                        expected_msg);
}

//________________________________________________________________________________________
// 
//  ambiguous_option
//
//________________________________________________________________________________________
void test_ambiguous_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("cfgfile1,c", value<string>(), "the config file") 
        ("cfgfile2,o",  value<string>(), "the config file") 
        ("good,g",                          "good option") 
        ("output,c",   value<string>(), "the output file") 
        ("output",     value<string>(), "the output file") 
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-ggc", "file", "-o", "anotherfile"}             ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfile", "file", "--cfgfile", "anotherfile"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-ggc", "file", "-o", "anotherfile"}             ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file", "/o", "anotherfile"}               ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "output=output.txt\n"}                                     ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "option '-c' is ambiguous and matches '--cfgfile1', and '--output'",
                                "option '--cfgfile' is ambiguous and matches '--cfgfile1', and '--cfgfile2'",
                                "option '-c' is ambiguous",
                                "option '/c' is ambiguous",
                                "option 'output' is ambiguous and matches different versions of 'output'",
    };
    test_exception_message<ambiguous_option>(argv, desc, "ambiguous_option",
                                             expected_msg);
}

//________________________________________________________________________________________
// 
//  multiple_occurrences
//
//________________________________________________________________________________________
void test_multiple_occurrences_exception_msg()
{
    options_description desc; 
    desc.add_options()
                ("cfgfile,c", value<string>(), "the configfile")
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-c", "file", "-c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfi", "file", "--cfgfi", "anotherfile"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-c", "file", "-c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file", "/c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "cfgfile=output.txt\ncfgfile=output.txt\n"}            ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "option '--cfgfile' cannot be specified more than once",
                                "option '--cfgfile' cannot be specified more than once",
                                "option '-c' cannot be specified more than once",
                                "option '/c' cannot be specified more than once",
                                "option 'cfgfile' cannot be specified more than once",
    };
    test_exception_message<multiple_occurrences>(argv, desc, "multiple_occurrences",
                                                expected_msg);
}

//________________________________________________________________________________________
// 
//  unknown_option
//
//________________________________________________________________________________________
void test_unknown_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("good,g",                          "good option") 
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-ggc", "file"}      ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfile", "file"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-ggc", "file"}      ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file"}        ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "cfgfile=output.txt\n"}        ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "unrecognised option '-ggc'",
                                "unrecognised option '--cfgfile'",
                                "unrecognised option '-ggc'",
                                "unrecognised option '/c'",
                                "unrecognised option 'cfgfile'",
    };
    test_exception_message<unknown_option>(argv, desc, "unknown_option", expected_msg);
}

//________________________________________________________________________________________
// 
//  validation_error::invalid_bool_value
//
//________________________________________________________________________________________
void test_invalid_bool_value_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("bool_option,b",	value< bool>(),	"bool_option")
        ;


    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--bool_optio", "file"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "bool_option=output.txt\n"}       ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "the argument ('file') for option '--bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '--bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '-b' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '/b' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('output.txt') for option 'bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::invalid_bool_value",
                                expected_msg);
}




//________________________________________________________________________________________
// 
//  validation_error::multiple_values_not_allowed
//
//________________________________________________________________________________________
//
//  Strange exception: sole purpose seems to be catching multitoken() associated with a scalar
//  validation_error::multiple_values_not_allowed seems thus to be a programmer error
//  
//
void test_multiple_values_not_allowed_exception_msg()
{
    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<string>()->multitoken(), "the config file")
         ("good,g",                                     "good option")
         ("output,o", value<string>(),                  "the output file")
       ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-c", "file", "c", "-o", "fritz", "hugo" }              ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfgfil", "file", "c", "--outpu", "fritz", "hugo" }   ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-c", "file", "c", "-o", "fritz", "hugo"}               ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/c", "file", "c", "/o", "fritz", "hugo"}               ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "" }                                                               ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "option '--cfgfile' only takes a single argument",
                                "option '--cfgfile' only takes a single argument",
                                "option '-c' only takes a single argument",
                                "option '/c' only takes a single argument",
                                "",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::multiple_values_not_allowed",
                                expected_msg);
}

//________________________________________________________________________________________
// 
//  validation_error::at_least_one_value_required
//
//________________________________________________________________________________________
//
//  Strange exception: sole purpose seems to be catching zero_tokens() associated with a scalar
//  validation_error::multiple_values_not_allowed seems thus to be a programmer error
//  
//
void test_at_least_one_value_required_exception_msg()
{


    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<int>()->zero_tokens(), "the config file")
         ("other,o", value<string>(), "other")
       ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-c"                       }  ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfg", "--o", "name"     }  ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-c"   , "-o"   , "name"   }  ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/c"                       }  ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { ""                                    }  ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "option '--cfgfile' requires at least one argument",
                                "option '--cfgfile' requires at least one argument",
                                "option '-c' requires at least one argument",
                                "option '/c' requires at least one argument",
                                "",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::at_least_one_value_required",
                                expected_msg);
}


//________________________________________________________________________________________
// 
//  required_option
//
//________________________________________________________________________________________
void test_required_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<string>()->required(), "the config file")
         ("good,g",                                 "good option")
         ("output,o", value<string>()->required(),  "the output file")
       ;

    vector<vector<const char*>> argv;            
    const char* argv0[] = { "program", "-g" }    ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--g" }   ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-g"}     ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/g"}     ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "" }                 ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "the option '--cfgfile' is required but missing",
                                "the option '--cfgfile' is required but missing",
                                "the option '-c' is required but missing",
                                "the option '/c' is required but missing",
                                "the option 'cfgfile' is required but missing",
    };
    test_exception_message<required_option>(argv, 
                                desc,
                                "required_option",
                                expected_msg);
}
















/** 
 * Check if this is the expected exception with the right message is being thrown inside 
 * func 
*/
template <typename EXCEPTION, typename FUNC>
void test_exception(const string& test_name, const string& exception_txt, FUNC func)
{

    try {
        options_description desc; 
        variables_map vm;
        func(desc, vm);
    } 
    catch (EXCEPTION& e)
    {
       CHECK_EQUAL(test_name, e.what(), exception_txt);
       return;
    }
    catch (std::exception& e)
    {
       BOOST_ERROR(string(test_name + ":\nUnexpected exception. ") + e.what() + 
                   "\nExpected text:\n" + exception_txt + "\n\n");
       return;
    }
    BOOST_ERROR(test_name + ": No exception thrown. ");
}



//________________________________________________________________________________________
// 
//  check_reading_file
//
//________________________________________________________________________________________
void check_reading_file(options_description& desc, variables_map& vm)
{
    desc.add_options()
         ("output,o", value<string>(), "the output file");
    
    const char* file_name = "no_such_file";
    store(parse_config_file<char>(file_name, desc, true), vm);

}


//________________________________________________________________________________________
// 
//  config_file_wildcard
//
//________________________________________________________________________________________
void config_file_wildcard(options_description& desc, variables_map& vm)
{
    desc.add_options()
         ("outpu*", value<string>(), "the output file1")
         ("outp*",    value<string>(), "the output file2")
       ;
    istringstream is("output1=whichone\noutput2=whichone\n");
    store(parse_config_file(is, desc), vm);
}

//________________________________________________________________________________________
// 
//  invalid_syntax::unrecognized_line
//
//________________________________________________________________________________________
void unrecognized_line(options_description& desc, variables_map& vm)
{
    istringstream is("funny wierd line\n");
    store(parse_config_file(is, desc), vm);
}

//________________________________________________________________________________________
// 
//  abbreviated_options_in_config_file
//
//________________________________________________________________________________________
void abbreviated_options_in_config_file(options_description& desc, variables_map& vm)
{
    desc.add_options()(",o", value<string>(), "the output file");
    istringstream is("o=output.txt\n");
    store(parse_config_file(is, desc), vm);
}


//________________________________________________________________________________________
// 
//  too_many_positional_options
//
//________________________________________________________________________________________
void too_many_positional_options(options_description& desc, variables_map& vm)
{
    const char* argv[] = {"program", "1", "2", "3"};
    positional_options_description positional_args;
    positional_args.add("two_positional_arguments", 2);
    store(command_line_parser(4, argv).options(desc).positional(positional_args).run(), vm);
}


//________________________________________________________________________________________
// 
//  invalid_command_line_style
//
//________________________________________________________________________________________

void test_invalid_command_line_style_exception_msg()
{
    string test_name = "invalid_command_line_style";
    using namespace command_line_style;
    options_description desc; 
    desc.add_options()("output,o", value<string>(), "the output file");

    vector<int> invalid_styles;
    invalid_styles.push_back(allow_short | short_allow_adjacent);
    invalid_styles.push_back(allow_short | allow_dash_for_short);
    invalid_styles.push_back(allow_long);
    vector<string> invalid_diagnostics;
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of command_line_style::allow_slash_for_short "
                                  "(slashes) or command_line_style::allow_dash_for_short "
                                  "(dashes) for short options.");
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of 'command_line_style::short_allow_next' "
                                  "(whitespace separated arguments) or "
                                  "command_line_style::short_allow_adjacent ('=' "
                                  "separated arguments) for short options.");
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of 'command_line_style::long_allow_next' "
                                  "(whitespace separated arguments) or "
                                  "command_line_style::long_allow_adjacent ('=' "
                                  "separated arguments) for long options.");


    const char* argv[] = {"program"};
    variables_map vm;
    for (unsigned ii = 0; ii < 3; ++ii)
    {
        bool exception_thrown = false;
        try
        {
            store(parse_command_line(1, argv, desc, invalid_styles[ii]), vm);
        }
        catch (invalid_command_line_style& e)
        {
           string error_msg("arguments are not allowed for unabbreviated option names");
           CHECK_EQUAL(test_name, e.what(), invalid_diagnostics[ii]);
           exception_thrown = true;
        }
        catch (std::exception& e)
        {
           BOOST_ERROR(string(test_name + ":\nUnexpected exception. ") + e.what() + 
                            "\nExpected text:\n" + invalid_diagnostics[ii] + "\n");
           exception_thrown = true;
        }
        if (!exception_thrown)
        {
            BOOST_ERROR(test_name << ": No exception thrown. ");
        }
    }
}



int main(int /*ac*/, char** /*av*/)
{
   test_ambiguous_option_exception_msg();
   test_unknown_option_exception_msg();
   test_multiple_occurrences_exception_msg();
   test_missing_value_exception_msg();
   test_invalid_option_value_exception_msg();
   test_invalid_bool_value_exception_msg();
   test_multiple_values_not_allowed_exception_msg();
   test_required_option_exception_msg();
   test_at_least_one_value_required_exception_msg();

   string test_name;
   string expected_message;


   // check_reading_file
   test_name        = "check_reading_file";
   expected_message = "can not read options configuration file 'no_such_file'";
   test_exception<reading_file>(test_name, expected_message, check_reading_file);

   // config_file_wildcard
   test_name        = "config_file_wildcard";
   expected_message = "options 'outpu*' and 'outp*' will both match the same arguments from the configuration file";
   test_exception<error>(test_name, expected_message, config_file_wildcard);

   // unrecognized_line
   test_name        = "unrecognized_line";
   expected_message = "the options configuration file contains an invalid line 'funny wierd line'";
   test_exception<invalid_syntax>(test_name, expected_message, unrecognized_line);


   // abbreviated_options_in_config_file
   test_name        = "abbreviated_options_in_config_file";
   expected_message = "abbreviated option names are not permitted in options configuration files";
   test_exception<error>(test_name, expected_message, abbreviated_options_in_config_file);

   test_name        = "too_many_positional_options";
   expected_message = "too many positional arguments have been specified on the command line";
   test_exception<too_many_positional_options_error>(
                          test_name, expected_message, too_many_positional_options);

   test_invalid_command_line_style_exception_msg();


   return 0;
}



// Copyright Sascha Ochsenknecht 2009.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <boost/program_options/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/cmdline.hpp>
using namespace boost::program_options;

#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>
using namespace std;

#include "minitest.hpp"



// 
//  like BOOST_CHECK_EQUAL but with more descriptive error message
//
#define CHECK_EQUAL(description, a, b) if (a != b) {std::cerr << "\n\nError:\n<<" << \
        description << ">>\n  Expected text=\"" << b << "\"\n  Actual text  =\"" << a << "\"\n\n"; assert(a == b);}


// Uncomment for Debugging, removes asserts so we can see more failures!
//#define BOOST_ERROR(description) std::cerr << description; std::cerr << "\n"; 


//8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888
// 
//  Uncomment to print out the complete set of diagnostic messages for the different test cases
/*
#define CHECK_EQUAL(description, a, b) if (a != b) {std::cerr << "\n\nError:  " << \
    description << "\n  Expecting\n" << b << "\n  Found\n" << a << "\n\n"; } \
    else {std::cout << description<< "\t" << b << "\n";}
*/ 

//8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888


// 
//  test exception for each specified command line style, e.g. short dash or config file
//
template<typename EXCEPTION>
void test_each_exception_message(const string& test_description, const vector<const char*>& argv, options_description& desc, int style, string exception_msg, istream& is = cin)
{
    if (exception_msg.length() == 0)
        return;
    variables_map vm;
    unsigned argc = argv.size();


    try {
        if (style == -1)
            store(parse_config_file(is, desc), vm);
        else
            store(parse_command_line(argv.size(), argv.data(), desc, style), vm);
        notify(vm);    
    } 
    catch (EXCEPTION& e)
    {
       //cerr << "Correct:\n\t" << e.what() << "\n";
       CHECK_EQUAL(test_description, e.what(), exception_msg);
       return;
    }
    catch (std::exception& e)
    {
        // concatenate argv without boost::algorithm::join
        string argv_txt;
        for (unsigned ii = 0; ii < argc - 1; ++ii)
            argv_txt += argv[ii] + string(" ");
        if (argc)
            argv_txt += argv[argc - 1];

       BOOST_ERROR("\n<<" + test_description + 
                    string(">>\n  Unexpected exception type!\n  Actual text  =\"") + e.what() + 
                    "\"\n  argv         =\"" + argv_txt +
                    "\"\n  Expected text=\"" + exception_msg + "\"\n");
       return;
    }
    BOOST_ERROR(test_description + ": No exception thrown. ");
}




// 
//  test exception messages for all command line styles (unix/long/short/slash/config file)
//
//      try each command line style in turn
const int unix_style = command_line_style::unix_style;
const int short_dash = command_line_style::allow_dash_for_short | command_line_style::allow_short | command_line_style::short_allow_adjacent | command_line_style::allow_sticky;
const int short_slash = command_line_style::allow_slash_for_short | command_line_style::allow_short | command_line_style::short_allow_adjacent;
const int long_dash = command_line_style::allow_long | command_line_style::long_allow_adjacent | command_line_style::allow_guessing;



template<typename EXCEPTION>
void test_exception_message(const vector<vector<const char*>>& argv,
                            options_description& desc,
                            const string& error_description, 
                            const char* expected_message_template[5])
{
    string expected_message;

    // unix
    expected_message = expected_message_template[0];
    test_each_exception_message<EXCEPTION>(error_description + " -- unix", 
                                  argv[0], desc, unix_style, expected_message);

    // long dash only
    expected_message = expected_message_template[1];
    test_each_exception_message<EXCEPTION>(error_description + " -- long_dash", 
                                  argv[1], desc, long_dash, expected_message);


    // short dash only
    expected_message = expected_message_template[2];
    test_each_exception_message<EXCEPTION>(error_description + " -- short_dash", 
                                  argv[2], desc, short_dash, expected_message);

    // short slash only
    expected_message = expected_message_template[3];
    test_each_exception_message<EXCEPTION>(error_description + " -- short_slash", 
                                  argv[3], desc, short_slash, expected_message);

    // config file only
    expected_message = expected_message_template[4];
    if (expected_message.length())
    {
        istringstream istrm(argv[4][0]);
        test_each_exception_message<EXCEPTION>(error_description + " -- config_file", 
                                          argv[4], desc, -1, expected_message, istrm);
    }

}

#define VEC_STR_PUSH_BACK(vec, c_array)  \
    vec.push_back(vector<const char*>(c_array, c_array + sizeof(c_array) / sizeof(char*)));

//________________________________________________________________________________________
// 
//  invalid_option_value
//
//________________________________________________________________________________________
void test_invalid_option_value_exception_msg()
{
    options_description desc;
    desc.add_options()
        ("int_option,d", value< int >(),	"An option taking an integer")
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--int", "A_STRING"};  VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "int_option=A_STRING"}         ;  VEC_STR_PUSH_BACK(argv, argv4); 

    const char* expected_msg[5] = {
                                "the argument ('A_STRING') for option '--int_option' is invalid", 
                                "the argument ('A_STRING') for option '--int_option' is invalid", 
                                "the argument ('A_STRING') for option '-d' is invalid", 
                                "the argument ('A_STRING') for option '/d' is invalid", 
                                "the argument ('A_STRING') for option 'int_option' is invalid"
    };


    test_exception_message<invalid_option_value>(argv, desc, "invalid_option_value",
                                                 expected_msg);


}

//________________________________________________________________________________________
// 
//  missing_value
//
//________________________________________________________________________________________
void test_missing_value_exception_msg()
{
    options_description desc;
    desc.add_options()
        ("cfgfile,e", value<string>(), "the config file") 
        ("output,o", value<string>(), "the output file")
    ;
    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-e", "-e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfgfile"}              ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-e", "-e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/e", "/e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { ""}    ;      VEC_STR_PUSH_BACK(argv, argv4); 

    const char* expected_msg[5] = {
                                "the required argument for option '--cfgfile' is missing",
                                "the required argument for option '--cfgfile' is missing",
                                "the required argument for option '-e' is missing",
                                "", // Ignore probable bug in cmdline::finish_option
                                    //"the required argument for option '/e' is missing",
                                "",
    };
    test_exception_message<invalid_command_line_syntax>(argv, desc, 
                                                        "invalid_syntax::missing_parameter", 
                                                        expected_msg);
}

//________________________________________________________________________________________
// 
//  ambiguous_option
//
//________________________________________________________________________________________
void test_ambiguous_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("cfgfile1,c", value<string>(), "the config file") 
        ("cfgfile2,o",  value<string>(), "the config file") 
        ("good,g",                          "good option") 
        ("output,c",   value<string>(), "the output file") 
        ("output",     value<string>(), "the output file") 
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-ggc", "file", "-o", "anotherfile"}             ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfile", "file", "--cfgfile", "anotherfile"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-ggc", "file", "-o", "anotherfile"}             ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file", "/o", "anotherfile"}               ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "output=output.txt\n"}                                     ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "option '-c' is ambiguous and matches '--cfgfile1', and '--output'",
                                "option '--cfgfile' is ambiguous and matches '--cfgfile1', and '--cfgfile2'",
                                "option '-c' is ambiguous",
                                "option '/c' is ambiguous",
                                "option 'output' is ambiguous and matches different versions of 'output'",
    };
    test_exception_message<ambiguous_option>(argv, desc, "ambiguous_option",
                                             expected_msg);
}

//________________________________________________________________________________________
// 
//  multiple_occurrences
//
//________________________________________________________________________________________
void test_multiple_occurrences_exception_msg()
{
    options_description desc; 
    desc.add_options()
                ("cfgfile,c", value<string>(), "the configfile")
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-c", "file", "-c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfi", "file", "--cfgfi", "anotherfile"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-c", "file", "-c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file", "/c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "cfgfile=output.txt\ncfgfile=output.txt\n"}            ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "option '--cfgfile' cannot be specified more than once",
                                "option '--cfgfile' cannot be specified more than once",
                                "option '-c' cannot be specified more than once",
                                "option '/c' cannot be specified more than once",
                                "option 'cfgfile' cannot be specified more than once",
    };
    test_exception_message<multiple_occurrences>(argv, desc, "multiple_occurrences",
                                                expected_msg);
}

//________________________________________________________________________________________
// 
//  unknown_option
//
//________________________________________________________________________________________
void test_unknown_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("good,g",                          "good option") 
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-ggc", "file"}      ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfile", "file"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-ggc", "file"}      ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file"}        ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "cfgfile=output.txt\n"}        ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "unrecognised option '-ggc'",
                                "unrecognised option '--cfgfile'",
                                "unrecognised option '-ggc'",
                                "unrecognised option '/c'",
                                "unrecognised option 'cfgfile'",
    };
    test_exception_message<unknown_option>(argv, desc, "unknown_option", expected_msg);
}

//________________________________________________________________________________________
// 
//  validation_error::invalid_bool_value
//
//________________________________________________________________________________________
void test_invalid_bool_value_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("bool_option,b",	value< bool>(),	"bool_option")
        ;


    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--bool_optio", "file"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "bool_option=output.txt\n"}       ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "the argument ('file') for option '--bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '--bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '-b' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '/b' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('output.txt') for option 'bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::invalid_bool_value",
                                expected_msg);
}




//________________________________________________________________________________________
// 
//  validation_error::multiple_values_not_allowed
//
//________________________________________________________________________________________
//
//  Strange exception: sole purpose seems to be catching multitoken() associated with a scalar
//  validation_error::multiple_values_not_allowed seems thus to be a programmer error
//  
//
void test_multiple_values_not_allowed_exception_msg()
{
    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<string>()->multitoken(), "the config file")
         ("good,g",                                     "good option")
         ("output,o", value<string>(),                  "the output file")
       ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-c", "file", "c", "-o", "fritz", "hugo" }              ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfgfil", "file", "c", "--outpu", "fritz", "hugo" }   ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-c", "file", "c", "-o", "fritz", "hugo"}               ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/c", "file", "c", "/o", "fritz", "hugo"}               ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "" }                                                               ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "option '--cfgfile' only takes a single argument",
                                "option '--cfgfile' only takes a single argument",
                                "option '-c' only takes a single argument",
                                "option '/c' only takes a single argument",
                                "",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::multiple_values_not_allowed",
                                expected_msg);
}

//________________________________________________________________________________________
// 
//  validation_error::at_least_one_value_required
//
//________________________________________________________________________________________
//
//  Strange exception: sole purpose seems to be catching zero_tokens() associated with a scalar
//  validation_error::multiple_values_not_allowed seems thus to be a programmer error
//  
//
void test_at_least_one_value_required_exception_msg()
{


    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<int>()->zero_tokens(), "the config file")
         ("other,o", value<string>(), "other")
       ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-c"                       }  ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfg", "--o", "name"     }  ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-c"   , "-o"   , "name"   }  ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/c"                       }  ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { ""                                    }  ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "option '--cfgfile' requires at least one argument",
                                "option '--cfgfile' requires at least one argument",
                                "option '-c' requires at least one argument",
                                "option '/c' requires at least one argument",
                                "",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::at_least_one_value_required",
                                expected_msg);
}


//________________________________________________________________________________________
// 
//  required_option
//
//________________________________________________________________________________________
void test_required_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<string>()->required(), "the config file")
         ("good,g",                                 "good option")
         ("output,o", value<string>()->required(),  "the output file")
       ;

    vector<vector<const char*>> argv;            
    const char* argv0[] = { "program", "-g" }    ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--g" }   ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-g"}     ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/g"}     ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "" }                 ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "the option '--cfgfile' is required but missing",
                                "the option '--cfgfile' is required but missing",
                                "the option '-c' is required but missing",
                                "the option '/c' is required but missing",
                                "the option 'cfgfile' is required but missing",
    };
    test_exception_message<required_option>(argv, 
                                desc,
                                "required_option",
                                expected_msg);
}
















/** 
 * Check if this is the expected exception with the right message is being thrown inside 
 * func 
*/
template <typename EXCEPTION, typename FUNC>
void test_exception(const string& test_name, const string& exception_txt, FUNC func)
{

    try {
        options_description desc; 
        variables_map vm;
        func(desc, vm);
    } 
    catch (EXCEPTION& e)
    {
       CHECK_EQUAL(test_name, e.what(), exception_txt);
       return;
    }
    catch (std::exception& e)
    {
       BOOST_ERROR(string(test_name + ":\nUnexpected exception. ") + e.what() + 
                   "\nExpected text:\n" + exception_txt + "\n\n");
       return;
    }
    BOOST_ERROR(test_name + ": No exception thrown. ");
}



//________________________________________________________________________________________
// 
//  check_reading_file
//
//________________________________________________________________________________________
void check_reading_file(options_description& desc, variables_map& vm)
{
    desc.add_options()
         ("output,o", value<string>(), "the output file");
    
    const char* file_name = "no_such_file";
    store(parse_config_file<char>(file_name, desc, true), vm);

}


//________________________________________________________________________________________
// 
//  config_file_wildcard
//
//________________________________________________________________________________________
void config_file_wildcard(options_description& desc, variables_map& vm)
{
    desc.add_options()
         ("outpu*", value<string>(), "the output file1")
         ("outp*",    value<string>(), "the output file2")
       ;
    istringstream is("output1=whichone\noutput2=whichone\n");
    store(parse_config_file(is, desc), vm);
}

//________________________________________________________________________________________
// 
//  invalid_syntax::unrecognized_line
//
//________________________________________________________________________________________
void unrecognized_line(options_description& desc, variables_map& vm)
{
    istringstream is("funny wierd line\n");
    store(parse_config_file(is, desc), vm);
}

//________________________________________________________________________________________
// 
//  abbreviated_options_in_config_file
//
//________________________________________________________________________________________
void abbreviated_options_in_config_file(options_description& desc, variables_map& vm)
{
    desc.add_options()(",o", value<string>(), "the output file");
    istringstream is("o=output.txt\n");
    store(parse_config_file(is, desc), vm);
}


//________________________________________________________________________________________
// 
//  too_many_positional_options
//
//________________________________________________________________________________________
void too_many_positional_options(options_description& desc, variables_map& vm)
{
    const char* argv[] = {"program", "1", "2", "3"};
    positional_options_description positional_args;
    positional_args.add("two_positional_arguments", 2);
    store(command_line_parser(4, argv).options(desc).positional(positional_args).run(), vm);
}


//________________________________________________________________________________________
// 
//  invalid_command_line_style
//
//________________________________________________________________________________________

void test_invalid_command_line_style_exception_msg()
{
    string test_name = "invalid_command_line_style";
    using namespace command_line_style;
    options_description desc; 
    desc.add_options()("output,o", value<string>(), "the output file");

    vector<int> invalid_styles;
    invalid_styles.push_back(allow_short | short_allow_adjacent);
    invalid_styles.push_back(allow_short | allow_dash_for_short);
    invalid_styles.push_back(allow_long);
    vector<string> invalid_diagnostics;
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of command_line_style::allow_slash_for_short "
                                  "(slashes) or command_line_style::allow_dash_for_short "
                                  "(dashes) for short options.");
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of 'command_line_style::short_allow_next' "
                                  "(whitespace separated arguments) or "
                                  "command_line_style::short_allow_adjacent ('=' "
                                  "separated arguments) for short options.");
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of 'command_line_style::long_allow_next' "
                                  "(whitespace separated arguments) or "
                                  "command_line_style::long_allow_adjacent ('=' "
                                  "separated arguments) for long options.");


    const char* argv[] = {"program"};
    variables_map vm;
    for (unsigned ii = 0; ii < 3; ++ii)
    {
        bool exception_thrown = false;
        try
        {
            store(parse_command_line(1, argv, desc, invalid_styles[ii]), vm);
        }
        catch (invalid_command_line_style& e)
        {
           string error_msg("arguments are not allowed for unabbreviated option names");
           CHECK_EQUAL(test_name, e.what(), invalid_diagnostics[ii]);
           exception_thrown = true;
        }
        catch (std::exception& e)
        {
           BOOST_ERROR(string(test_name + ":\nUnexpected exception. ") + e.what() + 
                            "\nExpected text:\n" + invalid_diagnostics[ii] + "\n");
           exception_thrown = true;
        }
        if (!exception_thrown)
        {
            BOOST_ERROR(test_name << ": No exception thrown. ");
        }
    }
}



int main(int /*ac*/, char** /*av*/)
{
   test_ambiguous_option_exception_msg();
   test_unknown_option_exception_msg();
   test_multiple_occurrences_exception_msg();
   test_missing_value_exception_msg();
   test_invalid_option_value_exception_msg();
   test_invalid_bool_value_exception_msg();
   test_multiple_values_not_allowed_exception_msg();
   test_required_option_exception_msg();
   test_at_least_one_value_required_exception_msg();

   string test_name;
   string expected_message;


   // check_reading_file
   test_name        = "check_reading_file";
   expected_message = "can not read options configuration file 'no_such_file'";
   test_exception<reading_file>(test_name, expected_message, check_reading_file);

   // config_file_wildcard
   test_name        = "config_file_wildcard";
   expected_message = "options 'outpu*' and 'outp*' will both match the same arguments from the configuration file";
   test_exception<error>(test_name, expected_message, config_file_wildcard);

   // unrecognized_line
   test_name        = "unrecognized_line";
   expected_message = "the options configuration file contains an invalid line 'funny wierd line'";
   test_exception<invalid_syntax>(test_name, expected_message, unrecognized_line);


   // abbreviated_options_in_config_file
   test_name        = "abbreviated_options_in_config_file";
   expected_message = "abbreviated option names are not permitted in options configuration files";
   test_exception<error>(test_name, expected_message, abbreviated_options_in_config_file);

   test_name        = "too_many_positional_options";
   expected_message = "too many positional arguments have been specified on the command line";
   test_exception<too_many_positional_options_error>(
                          test_name, expected_message, too_many_positional_options);

   test_invalid_command_line_style_exception_msg();


   return 0;
}



// Copyright Sascha Ochsenknecht 2009.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <boost/program_options/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/cmdline.hpp>
using namespace boost::program_options;

#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>
using namespace std;

#include "minitest.hpp"



// 
//  like BOOST_CHECK_EQUAL but with more descriptive error message
//
#define CHECK_EQUAL(description, a, b) if (a != b) {std::cerr << "\n\nError:\n<<" << \
        description << ">>\n  Expected text=\"" << b << "\"\n  Actual text  =\"" << a << "\"\n\n"; assert(a == b);}


// Uncomment for Debugging, removes asserts so we can see more failures!
//#define BOOST_ERROR(description) std::cerr << description; std::cerr << "\n"; 


//8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888
// 
//  Uncomment to print out the complete set of diagnostic messages for the different test cases
//
//#define CHECK_EQUAL(description, a, b) if (a != b) {std::cerr << "\n\nError:  " << \
//    description << "\n  Expecting\n" << b << "\n  Found\n" << a << "\n\n"; } \
//    else {std::cout << description<< "\t" << b << "\n";}

//8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888


// 
//  test exception for each specified command line style, e.g. short dash or config file
//
template<typename EXCEPTION>
void test_each_exception_message(const string& test_description, const vector<const char*>& argv, options_description& desc, int style, string exception_msg, istream& is = cin)
{
    if (exception_msg.length() == 0)
        return;
    variables_map vm;
    unsigned argc = argv.size();


    try {
        if (style == -1)
            store(parse_config_file(is, desc), vm);
        else
            store(parse_command_line(argv.size(), argv.data(), desc, style), vm);
        notify(vm);    
    } 
    catch (EXCEPTION& e)
    {
       //cerr << "Correct:\n\t" << e.what() << "\n";
       CHECK_EQUAL(test_description, e.what(), exception_msg);
       return;
    }
    catch (std::exception& e)
    {
        // concatenate argv without boost::algorithm::join
        string argv_txt;
        for (unsigned ii = 0; ii < argc - 1; ++ii)
            argv_txt += argv[ii] + string(" ");
        if (argc)
            argv_txt += argv[argc - 1];

       BOOST_ERROR("\n<<" + test_description + 
                    string(">>\n  Unexpected exception type!\n  Actual text  =\"") + e.what() + 
                    "\"\n  argv         =\"" + argv_txt +
                    "\"\n  Expected text=\"" + exception_msg + "\"\n");
       return;
    }
    BOOST_ERROR(test_description + ": No exception thrown. ");
}




// 
//  test exception messages for all command line styles (unix/long/short/slash/config file)
//
//      try each command line style in turn
const int unix_style = command_line_style::unix_style;
const int short_dash = command_line_style::allow_dash_for_short | command_line_style::allow_short | command_line_style::short_allow_adjacent | command_line_style::allow_sticky;
const int short_slash = command_line_style::allow_slash_for_short | command_line_style::allow_short | command_line_style::short_allow_adjacent;
const int long_dash = command_line_style::allow_long | command_line_style::long_allow_adjacent | command_line_style::allow_guessing;



template<typename EXCEPTION>
void test_exception_message(const vector<vector<const char*>>& argv,
                            options_description& desc,
                            const string& error_description, 
                            const char* expected_message_template[5])
{
    string expected_message;

    // unix
    expected_message = expected_message_template[0];
    test_each_exception_message<EXCEPTION>(error_description + " -- unix", 
                                  argv[0], desc, unix_style, expected_message);

    // long dash only
    expected_message = expected_message_template[1];
    test_each_exception_message<EXCEPTION>(error_description + " -- long_dash", 
                                  argv[1], desc, long_dash, expected_message);


    // short dash only
    expected_message = expected_message_template[2];
    test_each_exception_message<EXCEPTION>(error_description + " -- short_dash", 
                                  argv[2], desc, short_dash, expected_message);

    // short slash only
    expected_message = expected_message_template[3];
    test_each_exception_message<EXCEPTION>(error_description + " -- short_slash", 
                                  argv[3], desc, short_slash, expected_message);

    // config file only
    expected_message = expected_message_template[4];
    if (expected_message.length())
    {
        istringstream istrm(argv[4][0]);
        test_each_exception_message<EXCEPTION>(error_description + " -- config_file", 
                                          argv[4], desc, -1, expected_message, istrm);
    }

}

#define VEC_STR_PUSH_BACK(vec, c_array)  \
    vec.push_back(vector<const char*>(c_array, c_array + sizeof(c_array) / sizeof(char*)));

//________________________________________________________________________________________
// 
//  invalid_option_value
//
//________________________________________________________________________________________
void test_invalid_option_value_exception_msg()
{
    options_description desc;
    int required_value;
    desc.add_options()
        ("int_option,d", value< int >(),	"An option taking an integer")
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--int", "A_STRING"};  VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "int_option=A_STRING"}         ;  VEC_STR_PUSH_BACK(argv, argv4); 

    const char* expected_msg[5] = {
                                "the argument ('A_STRING') for option '--int_option' is invalid", 
                                "the argument ('A_STRING') for option '--int_option' is invalid", 
                                "the argument ('A_STRING') for option '-d' is invalid", 
                                "the argument ('A_STRING') for option '/d' is invalid", 
                                "the argument ('A_STRING') for option 'int_option' is invalid"
    };


    test_exception_message<invalid_option_value>(argv, desc, "invalid_option_value",
                                                 expected_msg);


}

//________________________________________________________________________________________
// 
//  missing_value
//
//________________________________________________________________________________________
void test_missing_value_exception_msg()
{
    options_description desc;
    desc.add_options()
        ("cfgfile,e", value<string>(), "the config file") 
        ("output,o", value<string>(), "the output file")
    ;
    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-e", "-e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfgfile"}              ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-e", "-e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/e", "/e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { ""}    ;      VEC_STR_PUSH_BACK(argv, argv4); 

    const char* expected_msg[5] = {
                                "the required argument for option '--cfgfile' is missing",
                                "the required argument for option '--cfgfile' is missing",
                                "the required argument for option '-e' is missing",
                                "", // Ignore probable bug in cmdline::finish_option
                                    //"the required argument for option '/e' is missing",
                                "",
    };
    test_exception_message<invalid_command_line_syntax>(argv, desc, 
                                                        "invalid_syntax::missing_parameter", 
                                                        expected_msg);
}

//________________________________________________________________________________________
// 
//  ambiguous_option
//
//________________________________________________________________________________________
void test_ambiguous_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("cfgfile1,c", value<string>()->multitoken(), "the config file") 
        ("cfgfile2,o",  value<string>(), "the config file") 
        ("good,g",                          "good option") 
        ("output,c",   value<string>(), "the output file") 
        ("output",     value<string>(), "the output file") 
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-ggc", "file", "-o", "anotherfile"}             ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfile", "file", "--cfgfile", "anotherfile"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-ggc", "file", "-o", "anotherfile"}             ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file", "/o", "anotherfile"}               ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "output=output.txt\n"}                                     ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "option '-c' is ambiguous and matches '--cfgfile1', and '--output'",
                                "option '--cfgfile' is ambiguous and matches '--cfgfile1', and '--cfgfile2'",
                                "option '-c' is ambiguous",
                                "option '/c' is ambiguous",
                                "option 'output' is ambiguous and matches different versions of 'output'",
    };
    test_exception_message<ambiguous_option>(argv, desc, "ambiguous_option",
                                             expected_msg);
}

//________________________________________________________________________________________
// 
//  multiple_occurrences
//
//________________________________________________________________________________________
void test_multiple_occurrences_exception_msg()
{
    options_description desc; 
    desc.add_options()
                ("cfgfile,c", value<string>(), "the configfile")
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-c", "file", "-c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfi", "file", "--cfgfi", "anotherfile"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-c", "file", "-c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file", "/c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "cfgfile=output.txt\ncfgfile=output.txt\n"}            ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "option '--cfgfile' cannot be specified more than once",
                                "option '--cfgfile' cannot be specified more than once",
                                "option '-c' cannot be specified more than once",
                                "option '/c' cannot be specified more than once",
                                "option 'cfgfile' cannot be specified more than once",
    };
    test_exception_message<multiple_occurrences>(argv, desc, "multiple_occurrences",
                                                expected_msg);
}

//________________________________________________________________________________________
// 
//  unknown_option
//
//________________________________________________________________________________________
void test_unknown_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("good,g",                          "good option") 
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-ggc", "file"}      ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfile", "file"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-ggc", "file"}      ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file"}        ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "cfgfile=output.txt\n"}        ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "unrecognised option '-ggc'",
                                "unrecognised option '--cfgfile'",
                                "unrecognised option '-ggc'",
                                "unrecognised option '/c'",
                                "unrecognised option 'cfgfile'",
    };
    test_exception_message<unknown_option>(argv, desc, "unknown_option", expected_msg);
}

//________________________________________________________________________________________
// 
//  validation_error::invalid_bool_value
//
//________________________________________________________________________________________
void test_invalid_bool_value_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("bool_option,b",	value< bool>(),	"bool_option")
        ;


    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--bool_optio", "file"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "bool_option=output.txt\n"}       ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "the argument ('file') for option '--bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '--bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '-b' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '/b' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('output.txt') for option 'bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::invalid_bool_value",
                                expected_msg);
}




//________________________________________________________________________________________
// 
//  validation_error::multiple_values_not_allowed
//
//________________________________________________________________________________________
//
//  Strange exception: sole purpose seems to be catching multitoken() not associated with a vector<string>??
//                   This is surely a corner case.
//
void test_multiple_values_not_allowed_exception_msg()
{
    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<string>()->multitoken(),   "the config file")
         ("good,g",                                     "good option")
         ("output,o", value<string>()->multitoken(),    "the output file")
       ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-c", "file", "c", "-o", "fritz", "hugo" }              ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfgfil", "file", "c", "--outpu", "fritz", "hugo" }   ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-c", "file", "c", "-o", "fritz", "hugo"}               ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/c", "file", "c", "/o", "fritz", "hugo"}               ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "" }                                                               ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "option '--cfgfile' only takes a single argument",
                                "option '--cfgfile' only takes a single argument",
                                "option '-c' only takes a single argument",
                                "option '/c' only takes a single argument",
                                "",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::multiple_values_not_allowed",
                                expected_msg);
}

//________________________________________________________________________________________
// 
//  validation_error::at_least_one_value_required
//
//________________________________________________________________________________________
//
//  Can't build a test case!!!
// 
// 
void test_at_least_one_value_required_exception_msg()
{

    return; 

    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<vector<int>>()->multitoken(), "the config file")
         ("other,o", value<string>(), "other")
       ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-c"                          }  ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfg"                       }  ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-c"   , "-o"   , "name"      }  ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/c"                          }  ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "" }                          ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "Unexpected exception. the required argument for option '--cfgfile' is missing",
                                "Unexpected exception. the required argument for option '--cfgfile' is missing",
                                "Unexpected exception. the required argument for option '-c' is missing",
                                "Unexpected exception. the required argument for option '/c' is missing",
                                "",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::at_least_one_value_required",
                                expected_msg);
}


//________________________________________________________________________________________
// 
//  required_option
//
//________________________________________________________________________________________
void test_required_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<string>()->required(), "the config file")
         ("good,g",                                 "good option")
         ("output,o", value<string>()->required(),  "the output file")
       ;

    vector<vector<const char*>> argv;            
    const char* argv0[] = { "program", "-g" }    ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--g" }   ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-g"}     ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/g"}     ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "" }                 ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "the option '--cfgfile' is required but missing",
                                "the option '--cfgfile' is required but missing",
                                "the option '-c' is required but missing",
                                "the option '/c' is required but missing",
                                "the option 'cfgfile' is required but missing",
    };
    test_exception_message<required_option>(argv, 
                                desc,
                                "required_option",
                                expected_msg);
}


















/** 
 * Check if this is the expected exception with the right message is being thrown inside 
 * func 
*/
template <typename EXCEPTION, typename FUNC>
void test_exception(const string& test_name, const string& exception_txt, FUNC func)
{

    try {
        options_description desc; 
        variables_map vm;
        func(desc, vm);
    } 
    catch (EXCEPTION& e)
    {
       CHECK_EQUAL(test_name, e.what(), exception_txt);
       return;
    }
    catch (std::exception& e)
    {
       BOOST_ERROR(string(test_name + ":\nUnexpected exception. ") + e.what() + 
                   "\nExpected text:\n" + exception_txt + "\n\n");
       return;
    }
    BOOST_ERROR(test_name + ": No exception thrown. ");
}



//________________________________________________________________________________________
// 
//  check_reading_file
//
//________________________________________________________________________________________
void check_reading_file(options_description& desc, variables_map& vm)
{
    desc.add_options()
         ("output,o", value<string>(), "the output file");
    
    const char* file_name = "no_such_file";
    store(parse_config_file<char>(file_name, desc, true), vm);

}


//________________________________________________________________________________________
// 
//  config_file_wildcard
//
//________________________________________________________________________________________
void config_file_wildcard(options_description& desc, variables_map& vm)
{
    desc.add_options()
         ("outpu*", value<string>(), "the output file1")
         ("outp*",    value<string>(), "the output file2")
       ;
    istringstream is("output1=whichone\noutput2=whichone\n");
    store(parse_config_file(is, desc), vm);
}

//________________________________________________________________________________________
// 
//  invalid_syntax::unrecognized_line
//
//________________________________________________________________________________________
void unrecognized_line(options_description& desc, variables_map& vm)
{
    istringstream is("funny wierd line\n");
    store(parse_config_file(is, desc), vm);
}

//________________________________________________________________________________________
// 
//  abbreviated_options_in_config_file
//
//________________________________________________________________________________________
void abbreviated_options_in_config_file(options_description& desc, variables_map& vm)
{
    desc.add_options()(",o", value<string>(), "the output file");
    istringstream is("o=output.txt\n");
    store(parse_config_file(is, desc), vm);
}


//________________________________________________________________________________________
// 
//  too_many_positional_options
//
//________________________________________________________________________________________
void too_many_positional_options(options_description& desc, variables_map& vm)
{
    const char* argv[] = {"program", "1", "2", "3"};
    positional_options_description positional_args;
    positional_args.add("two_positional_arguments", 2);
    store(command_line_parser(4, argv).options(desc).positional(positional_args).run(), vm);
}


//________________________________________________________________________________________
// 
//  invalid_command_line_style
//
//________________________________________________________________________________________

void test_invalid_command_line_style_exception_msg()
{
    string test_name = "invalid_command_line_style";
    using namespace command_line_style;
    unsigned basic_unix_style = (allow_short   
                                  | allow_long 
                                  | allow_sticky | allow_guessing 
                                  | allow_dash_for_short);
    options_description desc; 
    desc.add_options()("output,o", value<string>(), "the output file");

    vector<int> invalid_styles;
    invalid_styles.push_back(allow_short | short_allow_adjacent);
    invalid_styles.push_back(allow_short | allow_dash_for_short);
    invalid_styles.push_back(allow_long);
    vector<string> invalid_diagnostics;
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of command_line_style::allow_slash_for_short "
                                  "(slashes) or command_line_style::allow_dash_for_short "
                                  "(dashes) for short options.");
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of 'command_line_style::short_allow_next' "
                                  "(whitespace separated arguments) or "
                                  "command_line_style::short_allow_adjacent ('=' "
                                  "separated arguments) for short options.");
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of 'command_line_style::long_allow_next' "
                                  "(whitespace separated arguments) or "
                                  "command_line_style::long_allow_adjacent ('=' "
                                  "separated arguments) for long options.");


    const char* argv[] = {"program"};
    variables_map vm;
    for (unsigned ii = 0; ii < 3; ++ii)
    {
        bool exception_thrown = false;
        try
        {
            store(parse_command_line(1, argv, desc, invalid_styles[ii]), vm);
        }
        catch (invalid_command_line_style& e)
        {
           string error_msg("arguments are not allowed for unabbreviated option names");
           CHECK_EQUAL(test_name, e.what(), invalid_diagnostics[ii]);
           exception_thrown = true;
        }
        catch (std::exception& e)
        {
           BOOST_ERROR(string(test_name + ":\nUnexpected exception. ") + e.what() + 
                            "\nExpected text:\n" + invalid_diagnostics[ii] + "\n");
           exception_thrown = true;
        }
        if (!exception_thrown)
        {
            BOOST_ERROR(test_name << ": No exception thrown. ");
        }
    }
}


int main(int /*ac*/, char** /*av*/)
{
   test_ambiguous_option_exception_msg();
   test_unknown_option_exception_msg();
   test_multiple_occurrences_exception_msg();
   test_missing_value_exception_msg();
   test_invalid_option_value_exception_msg();
   test_invalid_bool_value_exception_msg();
   test_multiple_values_not_allowed_exception_msg();
   test_required_option_exception_msg();
   test_at_least_one_value_required_exception_msg();


   string test_name;
   string expected_message;


   // check_reading_file
   test_name        = "check_reading_file";
   expected_message = "can not read options configuration file 'no_such_file'";
   test_exception<reading_file>(test_name, expected_message, check_reading_file);

   // config_file_wildcard
   test_name        = "config_file_wildcard";
   expected_message = "options 'outpu*' and 'outp*' will both match the same arguments from the configuration file";
   test_exception<error>(test_name, expected_message, config_file_wildcard);

   // unrecognized_line
   test_name        = "unrecognized_line";
   expected_message = "the options configuration file contains an invalid line 'funny wierd line'";
   test_exception<invalid_syntax>(test_name, expected_message, unrecognized_line);


   // abbreviated_options_in_config_file
   test_name        = "abbreviated_options_in_config_file";
   expected_message = "abbreviated option names are not permitted in options configuration files";
   test_exception<error>(test_name, expected_message, abbreviated_options_in_config_file);

   test_name        = "too_many_positional_options";
   expected_message = "too many positional arguments have been specified on the command line";
   test_exception<too_many_positional_options_error>(
                          test_name, expected_message, too_many_positional_options);

   test_invalid_command_line_style_exception_msg();
   return 0;
}



// Copyright Sascha Ochsenknecht 2009.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <boost/program_options/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/cmdline.hpp>
using namespace boost::program_options;

#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>
using namespace std;

#include "minitest.hpp"



// 
//  like BOOST_CHECK_EQUAL but with more descriptive error message
//
#define CHECK_EQUAL(description, a, b) if (a != b) {std::cerr << "\n\nError:\n<<" << \
        description << ">>\n  Expected text=\"" << b << "\"\n  Actual text  =\"" << a << "\"\n\n"; assert(a == b);}


// Uncomment for Debugging, removes asserts so we can see more failures!
//#define BOOST_ERROR(description) std::cerr << description; std::cerr << "\n"; 


//8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888
// 
//  Uncomment to print out the complete set of diagnostic messages for the different test cases
//
//#define CHECK_EQUAL(description, a, b) if (a != b) {std::cerr << "\n\nError:  " << \
//    description << "\n  Expecting\n" << b << "\n  Found\n" << a << "\n\n"; } \
//    else {std::cout << description<< "\t" << b << "\n";}

//8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888


// 
//  test exception for each specified command line style, e.g. short dash or config file
//
template<typename EXCEPTION>
void test_each_exception_message(const string& test_description, const vector<const char*>& argv, options_description& desc, int style, string exception_msg, istream& is = cin)
{
    if (exception_msg.length() == 0)
        return;
    variables_map vm;
    unsigned argc = argv.size();


    try {
        if (style == -1)
            store(parse_config_file(is, desc), vm);
        else
            store(parse_command_line(argv.size(), argv.data(), desc, style), vm);
        notify(vm);    
    } 
    catch (EXCEPTION& e)
    {
       //cerr << "Correct:\n\t" << e.what() << "\n";
       CHECK_EQUAL(test_description, e.what(), exception_msg);
       return;
    }
    catch (std::exception& e)
    {
        // concatenate argv without boost::algorithm::join
        string argv_txt;
        for (unsigned ii = 0; ii < argc - 1; ++ii)
            argv_txt += argv[ii] + string(" ");
        if (argc)
            argv_txt += argv[argc - 1];

       BOOST_ERROR("\n<<" + test_description + 
                    string(">>\n  Unexpected exception type!\n  Actual text  =\"") + e.what() + 
                    "\"\n  argv         =\"" + argv_txt +
                    "\"\n  Expected text=\"" + exception_msg + "\"\n");
       return;
    }
    BOOST_ERROR(test_description + ": No exception thrown. ");
}




// 
//  test exception messages for all command line styles (unix/long/short/slash/config file)
//
//      try each command line style in turn
const int unix_style = command_line_style::unix_style;
const int short_dash = command_line_style::allow_dash_for_short | command_line_style::allow_short | command_line_style::short_allow_adjacent | command_line_style::allow_sticky;
const int short_slash = command_line_style::allow_slash_for_short | command_line_style::allow_short | command_line_style::short_allow_adjacent;
const int long_dash = command_line_style::allow_long | command_line_style::long_allow_adjacent | command_line_style::allow_guessing;



template<typename EXCEPTION>
void test_exception_message(const vector<vector<const char*>>& argv,
                            options_description& desc,
                            const string& error_description, 
                            const char* expected_message_template[5])
{
    string expected_message;

    // unix
    expected_message = expected_message_template[0];
    test_each_exception_message<EXCEPTION>(error_description + " -- unix", 
                                  argv[0], desc, unix_style, expected_message);

    // long dash only
    expected_message = expected_message_template[1];
    test_each_exception_message<EXCEPTION>(error_description + " -- long_dash", 
                                  argv[1], desc, long_dash, expected_message);


    // short dash only
    expected_message = expected_message_template[2];
    test_each_exception_message<EXCEPTION>(error_description + " -- short_dash", 
                                  argv[2], desc, short_dash, expected_message);

    // short slash only
    expected_message = expected_message_template[3];
    test_each_exception_message<EXCEPTION>(error_description + " -- short_slash", 
                                  argv[3], desc, short_slash, expected_message);

    // config file only
    expected_message = expected_message_template[4];
    if (expected_message.length())
    {
        istringstream istrm(argv[4][0]);
        test_each_exception_message<EXCEPTION>(error_description + " -- config_file", 
                                          argv[4], desc, -1, expected_message, istrm);
    }

}

#define VEC_STR_PUSH_BACK(vec, c_array)  \
    vec.push_back(vector<const char*>(c_array, c_array + sizeof(c_array) / sizeof(char*)));

//________________________________________________________________________________________
// 
//  invalid_option_value
//
//________________________________________________________________________________________
void test_invalid_option_value_exception_msg()
{
    options_description desc;
    int required_value;
    desc.add_options()
        ("int_option,d", value< int >(),	"An option taking an integer")
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--int", "A_STRING"};  VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "int_option=A_STRING"}         ;  VEC_STR_PUSH_BACK(argv, argv4); 

    const char* expected_msg[5] = {
                                "the argument ('A_STRING') for option '--int_option' is invalid", 
                                "the argument ('A_STRING') for option '--int_option' is invalid", 
                                "the argument ('A_STRING') for option '-d' is invalid", 
                                "the argument ('A_STRING') for option '/d' is invalid", 
                                "the argument ('A_STRING') for option 'int_option' is invalid"
    };


    test_exception_message<invalid_option_value>(argv, desc, "invalid_option_value",
                                                 expected_msg);


}

//________________________________________________________________________________________
// 
//  missing_value
//
//________________________________________________________________________________________
void test_missing_value_exception_msg()
{
    options_description desc;
    desc.add_options()
        ("cfgfile,e", value<string>(), "the config file") 
        ("output,o", value<string>(), "the output file")
    ;
    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-e", "-e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfgfile"}              ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-e", "-e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/e", "/e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { ""}    ;      VEC_STR_PUSH_BACK(argv, argv4); 

    const char* expected_msg[5] = {
                                "the required argument for option '--cfgfile' is missing",
                                "the required argument for option '--cfgfile' is missing",
                                "the required argument for option '-e' is missing",
                                "", // Ignore probable bug in cmdline::finish_option
                                    //"the required argument for option '/e' is missing",
                                "",
    };
    test_exception_message<invalid_command_line_syntax>(argv, desc, 
                                                        "invalid_syntax::missing_parameter", 
                                                        expected_msg);
}

//________________________________________________________________________________________
// 
//  ambiguous_option
//
//________________________________________________________________________________________
void test_ambiguous_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("cfgfile1,c", value<string>()->multitoken(), "the config file") 
        ("cfgfile2,o",  value<string>(), "the config file") 
        ("good,g",                          "good option") 
        ("output,c",   value<string>(), "the output file") 
        ("output",     value<string>(), "the output file") 
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-ggc", "file", "-o", "anotherfile"}             ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfile", "file", "--cfgfile", "anotherfile"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-ggc", "file", "-o", "anotherfile"}             ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file", "/o", "anotherfile"}               ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "output=output.txt\n"}                                     ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "option '-c' is ambiguous and matches '--cfgfile1', and '--output'",
                                "option '--cfgfile' is ambiguous and matches '--cfgfile1', and '--cfgfile2'",
                                "option '-c' is ambiguous",
                                "option '/c' is ambiguous",
                                "option 'output' is ambiguous and matches different versions of 'output'",
    };
    test_exception_message<ambiguous_option>(argv, desc, "ambiguous_option",
                                             expected_msg);
}

//________________________________________________________________________________________
// 
//  multiple_occurrences
//
//________________________________________________________________________________________
void test_multiple_occurrences_exception_msg()
{
    options_description desc; 
    desc.add_options()
                ("cfgfile,c", value<string>(), "the configfile")
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-c", "file", "-c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfi", "file", "--cfgfi", "anotherfile"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-c", "file", "-c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file", "/c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "cfgfile=output.txt\ncfgfile=output.txt\n"}            ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "option '--cfgfile' cannot be specified more than once",
                                "option '--cfgfile' cannot be specified more than once",
                                "option '-c' cannot be specified more than once",
                                "option '/c' cannot be specified more than once",
                                "option 'cfgfile' cannot be specified more than once",
    };
    test_exception_message<multiple_occurrences>(argv, desc, "multiple_occurrences",
                                                expected_msg);
}

//________________________________________________________________________________________
// 
//  unknown_option
//
//________________________________________________________________________________________
void test_unknown_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("good,g",                          "good option") 
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-ggc", "file"}      ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfile", "file"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-ggc", "file"}      ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file"}        ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "cfgfile=output.txt\n"}        ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "unrecognised option '-ggc'",
                                "unrecognised option '--cfgfile'",
                                "unrecognised option '-ggc'",
                                "unrecognised option '/c'",
                                "unrecognised option 'cfgfile'",
    };
    test_exception_message<unknown_option>(argv, desc, "unknown_option", expected_msg);
}

//________________________________________________________________________________________
// 
//  validation_error::invalid_bool_value
//
//________________________________________________________________________________________
void test_invalid_bool_value_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("bool_option,b",	value< bool>(),	"bool_option")
        ;


    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--bool_optio", "file"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "bool_option=output.txt\n"}       ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "the argument ('file') for option '--bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '--bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '-b' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '/b' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('output.txt') for option 'bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::invalid_bool_value",
                                expected_msg);
}




//________________________________________________________________________________________
// 
//  validation_error::multiple_values_not_allowed
//
//________________________________________________________________________________________
//
//  Strange exception: sole purpose seems to be catching multitoken() not associated with a vector<string>??
//                   This is surely a corner case.
//
void test_multiple_values_not_allowed_exception_msg()
{
    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<string>()->multitoken(),   "the config file")
         ("good,g",                                     "good option")
         ("output,o", value<string>()->multitoken(),    "the output file")
       ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-c", "file", "c", "-o", "fritz", "hugo" }              ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfgfil", "file", "c", "--outpu", "fritz", "hugo" }   ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-c", "file", "c", "-o", "fritz", "hugo"}               ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/c", "file", "c", "/o", "fritz", "hugo"}               ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "" }                                                               ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "option '--cfgfile' only takes a single argument",
                                "option '--cfgfile' only takes a single argument",
                                "option '-c' only takes a single argument",
                                "option '/c' only takes a single argument",
                                "",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::multiple_values_not_allowed",
                                expected_msg);
}

//________________________________________________________________________________________
// 
//  validation_error::at_least_one_value_required
//
//________________________________________________________________________________________
//
//  Can't build a test case!!!
// 
// 
void test_at_least_one_value_required_exception_msg()
{

    return; 

    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<vector<int>>()->multitoken(), "the config file")
         ("other,o", value<string>(), "other")
       ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-c"                          }  ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfg"                       }  ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-c"   , "-o"   , "name"      }  ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/c"                          }  ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "" }                          ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "Unexpected exception. the required argument for option '--cfgfile' is missing",
                                "Unexpected exception. the required argument for option '--cfgfile' is missing",
                                "Unexpected exception. the required argument for option '-c' is missing",
                                "Unexpected exception. the required argument for option '/c' is missing",
                                "",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::at_least_one_value_required",
                                expected_msg);
}


//________________________________________________________________________________________
// 
//  required_option
//
//________________________________________________________________________________________
void test_required_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<string>()->required(), "the config file")
         ("good,g",                                 "good option")
         ("output,o", value<string>()->required(),  "the output file")
       ;

    vector<vector<const char*>> argv;            
    const char* argv0[] = { "program", "-g" }    ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--g" }   ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-g"}     ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/g"}     ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "" }                 ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "the option '--cfgfile' is required but missing",
                                "the option '--cfgfile' is required but missing",
                                "the option '-c' is required but missing",
                                "the option '/c' is required but missing",
                                "the option 'cfgfile' is required but missing",
    };
    test_exception_message<required_option>(argv, 
                                desc,
                                "required_option",
                                expected_msg);
}


















/** 
 * Check if this is the expected exception with the right message is being thrown inside 
 * func 
*/
template <typename EXCEPTION, typename FUNC>
void test_exception(const string& test_name, const string& exception_txt, FUNC func)
{

    try {
        options_description desc; 
        variables_map vm;
        func(desc, vm);
    } 
    catch (EXCEPTION& e)
    {
       CHECK_EQUAL(test_name, e.what(), exception_txt);
       return;
    }
    catch (std::exception& e)
    {
       BOOST_ERROR(string(test_name + ":\nUnexpected exception. ") + e.what() + 
                   "\nExpected text:\n" + exception_txt + "\n\n");
       return;
    }
    BOOST_ERROR(test_name + ": No exception thrown. ");
}



//________________________________________________________________________________________
// 
//  check_reading_file
//
//________________________________________________________________________________________
void check_reading_file(options_description& desc, variables_map& vm)
{
    desc.add_options()
         ("output,o", value<string>(), "the output file");
    
    const char* file_name = "no_such_file";
    store(parse_config_file<char>(file_name, desc, true), vm);

}


//________________________________________________________________________________________
// 
//  config_file_wildcard
//
//________________________________________________________________________________________
void config_file_wildcard(options_description& desc, variables_map& vm)
{
    desc.add_options()
         ("outpu*", value<string>(), "the output file1")
         ("outp*",    value<string>(), "the output file2")
       ;
    istringstream is("output1=whichone\noutput2=whichone\n");
    store(parse_config_file(is, desc), vm);
}

//________________________________________________________________________________________
// 
//  invalid_syntax::unrecognized_line
//
//________________________________________________________________________________________
void unrecognized_line(options_description& desc, variables_map& vm)
{
    istringstream is("funny wierd line\n");
    store(parse_config_file(is, desc), vm);
}

//________________________________________________________________________________________
// 
//  abbreviated_options_in_config_file
//
//________________________________________________________________________________________
void abbreviated_options_in_config_file(options_description& desc, variables_map& vm)
{
    desc.add_options()(",o", value<string>(), "the output file");
    istringstream is("o=output.txt\n");
    store(parse_config_file(is, desc), vm);
}


//________________________________________________________________________________________
// 
//  too_many_positional_options
//
//________________________________________________________________________________________
void too_many_positional_options(options_description& desc, variables_map& vm)
{
    const char* argv[] = {"program", "1", "2", "3"};
    positional_options_description positional_args;
    positional_args.add("two_positional_arguments", 2);
    store(command_line_parser(4, argv).options(desc).positional(positional_args).run(), vm);
}


//________________________________________________________________________________________
// 
//  invalid_command_line_style
//
//________________________________________________________________________________________

void test_invalid_command_line_style_exception_msg()
{
    string test_name = "invalid_command_line_style";
    using namespace command_line_style;
    unsigned basic_unix_style = (allow_short   
                                  | allow_long 
                                  | allow_sticky | allow_guessing 
                                  | allow_dash_for_short);
    options_description desc; 
    desc.add_options()("output,o", value<string>(), "the output file");

    vector<int> invalid_styles;
    invalid_styles.push_back(allow_short | short_allow_adjacent);
    invalid_styles.push_back(allow_short | allow_dash_for_short);
    invalid_styles.push_back(allow_long);
    vector<string> invalid_diagnostics;
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of command_line_style::allow_slash_for_short "
                                  "(slashes) or command_line_style::allow_dash_for_short "
                                  "(dashes) for short options.");
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of 'command_line_style::short_allow_next' "
                                  "(whitespace separated arguments) or "
                                  "command_line_style::short_allow_adjacent ('=' "
                                  "separated arguments) for short options.");
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of 'command_line_style::long_allow_next' "
                                  "(whitespace separated arguments) or "
                                  "command_line_style::long_allow_adjacent ('=' "
                                  "separated arguments) for long options.");


    const char* argv[] = {"program"};
    variables_map vm;
    for (unsigned ii = 0; ii < 3; ++ii)
    {
        bool exception_thrown = false;
        try
        {
            store(parse_command_line(1, argv, desc, invalid_styles[ii]), vm);
        }
        catch (invalid_command_line_style& e)
        {
           string error_msg("arguments are not allowed for unabbreviated option names");
           CHECK_EQUAL(test_name, e.what(), invalid_diagnostics[ii]);
           exception_thrown = true;
        }
        catch (std::exception& e)
        {
           BOOST_ERROR(string(test_name + ":\nUnexpected exception. ") + e.what() + 
                            "\nExpected text:\n" + invalid_diagnostics[ii] + "\n");
           exception_thrown = true;
        }
        if (!exception_thrown)
        {
            BOOST_ERROR(test_name << ": No exception thrown. ");
        }
    }
}


int main(int /*ac*/, char** /*av*/)
{
   test_ambiguous_option_exception_msg();
   test_unknown_option_exception_msg();
   test_multiple_occurrences_exception_msg();
   test_missing_value_exception_msg();
   test_invalid_option_value_exception_msg();
   test_invalid_bool_value_exception_msg();
   test_multiple_values_not_allowed_exception_msg();
   test_required_option_exception_msg();
   test_at_least_one_value_required_exception_msg();


   string test_name;
   string expected_message;


   // check_reading_file
   test_name        = "check_reading_file";
   expected_message = "can not read options configuration file 'no_such_file'";
   test_exception<reading_file>(test_name, expected_message, check_reading_file);

   // config_file_wildcard
   test_name        = "config_file_wildcard";
   expected_message = "options 'outpu*' and 'outp*' will both match the same arguments from the configuration file";
   test_exception<error>(test_name, expected_message, config_file_wildcard);

   // unrecognized_line
   test_name        = "unrecognized_line";
   expected_message = "the options configuration file contains an invalid line 'funny wierd line'";
   test_exception<invalid_syntax>(test_name, expected_message, unrecognized_line);


   // abbreviated_options_in_config_file
   test_name        = "abbreviated_options_in_config_file";
   expected_message = "abbreviated option names are not permitted in options configuration files";
   test_exception<error>(test_name, expected_message, abbreviated_options_in_config_file);

   test_name        = "too_many_positional_options";
   expected_message = "too many positional arguments have been specified on the command line";
   test_exception<too_many_positional_options_error>(
                          test_name, expected_message, too_many_positional_options);

   test_invalid_command_line_style_exception_msg();
   return 0;
}



// Copyright Sascha Ochsenknecht 2009.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <boost/program_options/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/cmdline.hpp>
using namespace boost::program_options;

#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>
using namespace std;

#include "minitest.hpp"



// 
//  like BOOST_CHECK_EQUAL but with more descriptive error message
//
#define CHECK_EQUAL(description, a, b) if (a != b) {std::cerr << "\n\nError:\n<<" << \
        description << ">>\n  Expected text=\"" << b << "\"\n  Actual text  =\"" << a << "\"\n\n"; assert(a == b);}


// Uncomment for Debugging, removes asserts so we can see more failures!
//#define BOOST_ERROR(description) std::cerr << description; std::cerr << "\n"; 


//8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888
// 
//  Uncomment to print out the complete set of diagnostic messages for the different test cases
//
//#define CHECK_EQUAL(description, a, b) if (a != b) {std::cerr << "\n\nError:  " << \
//    description << "\n  Expecting\n" << b << "\n  Found\n" << a << "\n\n"; } \
//    else {std::cout << description<< "\t" << b << "\n";}

//8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888


// 
//  test exception for each specified command line style, e.g. short dash or config file
//
template<typename EXCEPTION>
void test_each_exception_message(const string& test_description, const vector<const char*>& argv, options_description& desc, int style, string exception_msg, istream& is = cin)
{
    if (exception_msg.length() == 0)
        return;
    variables_map vm;
    unsigned argc = argv.size();


    try {
        if (style == -1)
            store(parse_config_file(is, desc), vm);
        else
            store(parse_command_line(argv.size(), argv.data(), desc, style), vm);
        notify(vm);    
    } 
    catch (EXCEPTION& e)
    {
       //cerr << "Correct:\n\t" << e.what() << "\n";
       CHECK_EQUAL(test_description, e.what(), exception_msg);
       return;
    }
    catch (std::exception& e)
    {
        // concatenate argv without boost::algorithm::join
        string argv_txt;
        for (unsigned ii = 0; ii < argc - 1; ++ii)
            argv_txt += argv[ii] + string(" ");
        if (argc)
            argv_txt += argv[argc - 1];

       BOOST_ERROR("\n<<" + test_description + 
                    string(">>\n  Unexpected exception type!\n  Actual text  =\"") + e.what() + 
                    "\"\n  argv         =\"" + argv_txt +
                    "\"\n  Expected text=\"" + exception_msg + "\"\n");
       return;
    }
    BOOST_ERROR(test_description + ": No exception thrown. ");
}




// 
//  test exception messages for all command line styles (unix/long/short/slash/config file)
//
//      try each command line style in turn
const int unix_style = command_line_style::unix_style;
const int short_dash = command_line_style::allow_dash_for_short | command_line_style::allow_short | command_line_style::short_allow_adjacent | command_line_style::allow_sticky;
const int short_slash = command_line_style::allow_slash_for_short | command_line_style::allow_short | command_line_style::short_allow_adjacent;
const int long_dash = command_line_style::allow_long | command_line_style::long_allow_adjacent | command_line_style::allow_guessing;



template<typename EXCEPTION>
void test_exception_message(const vector<vector<const char*>>& argv,
                            options_description& desc,
                            const string& error_description, 
                            const char* expected_message_template[5])
{
    string expected_message;

    // unix
    expected_message = expected_message_template[0];
    test_each_exception_message<EXCEPTION>(error_description + " -- unix", 
                                  argv[0], desc, unix_style, expected_message);

    // long dash only
    expected_message = expected_message_template[1];
    test_each_exception_message<EXCEPTION>(error_description + " -- long_dash", 
                                  argv[1], desc, long_dash, expected_message);


    // short dash only
    expected_message = expected_message_template[2];
    test_each_exception_message<EXCEPTION>(error_description + " -- short_dash", 
                                  argv[2], desc, short_dash, expected_message);

    // short slash only
    expected_message = expected_message_template[3];
    test_each_exception_message<EXCEPTION>(error_description + " -- short_slash", 
                                  argv[3], desc, short_slash, expected_message);

    // config file only
    expected_message = expected_message_template[4];
    if (expected_message.length())
    {
        istringstream istrm(argv[4][0]);
        test_each_exception_message<EXCEPTION>(error_description + " -- config_file", 
                                          argv[4], desc, -1, expected_message, istrm);
    }

}

#define VEC_STR_PUSH_BACK(vec, c_array)  \
    vec.push_back(vector<const char*>(c_array, c_array + sizeof(c_array) / sizeof(char*)));

//________________________________________________________________________________________
// 
//  invalid_option_value
//
//________________________________________________________________________________________
void test_invalid_option_value_exception_msg()
{
    options_description desc;
    int required_value;
    desc.add_options()
        ("int_option,d", value< int >(),	"An option taking an integer")
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--int", "A_STRING"};  VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/d", "A_STRING"}   ;  VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "int_option=A_STRING"}         ;  VEC_STR_PUSH_BACK(argv, argv4); 

    const char* expected_msg[5] = {
                                "the argument ('A_STRING') for option '--int_option' is invalid", 
                                "the argument ('A_STRING') for option '--int_option' is invalid", 
                                "the argument ('A_STRING') for option '-d' is invalid", 
                                "the argument ('A_STRING') for option '/d' is invalid", 
                                "the argument ('A_STRING') for option 'int_option' is invalid"
    };


    test_exception_message<invalid_option_value>(argv, desc, "invalid_option_value",
                                                 expected_msg);


}

//________________________________________________________________________________________
// 
//  missing_value
//
//________________________________________________________________________________________
void test_missing_value_exception_msg()
{
    options_description desc;
    desc.add_options()
        ("cfgfile,e", value<string>(), "the config file") 
        ("output,o", value<string>(), "the output file")
    ;
    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-e", "-e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfgfile"}              ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-e", "-e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/e", "/e", "output.txt"} ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { ""}    ;      VEC_STR_PUSH_BACK(argv, argv4); 

    const char* expected_msg[5] = {
                                "the required argument for option '--cfgfile' is missing",
                                "the required argument for option '--cfgfile' is missing",
                                "the required argument for option '-e' is missing",
                                "", // Ignore probable bug in cmdline::finish_option
                                    //"the required argument for option '/e' is missing",
                                "",
    };
    test_exception_message<invalid_command_line_syntax>(argv, desc, 
                                                        "invalid_syntax::missing_parameter", 
                                                        expected_msg);
}

//________________________________________________________________________________________
// 
//  ambiguous_option
//
//________________________________________________________________________________________
void test_ambiguous_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("cfgfile1,c", value<string>()->multitoken(), "the config file") 
        ("cfgfile2,o",  value<string>(), "the config file") 
        ("good,g",                          "good option") 
        ("output,c",   value<string>(), "the output file") 
        ("output",     value<string>(), "the output file") 
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-ggc", "file", "-o", "anotherfile"}             ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfile", "file", "--cfgfile", "anotherfile"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-ggc", "file", "-o", "anotherfile"}             ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file", "/o", "anotherfile"}               ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "output=output.txt\n"}                                     ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "option '-c' is ambiguous and matches '--cfgfile1', and '--output'",
                                "option '--cfgfile' is ambiguous and matches '--cfgfile1', and '--cfgfile2'",
                                "option '-c' is ambiguous",
                                "option '/c' is ambiguous",
                                "option 'output' is ambiguous and matches different versions of 'output'",
    };
    test_exception_message<ambiguous_option>(argv, desc, "ambiguous_option",
                                             expected_msg);
}

//________________________________________________________________________________________
// 
//  multiple_occurrences
//
//________________________________________________________________________________________
void test_multiple_occurrences_exception_msg()
{
    options_description desc; 
    desc.add_options()
                ("cfgfile,c", value<string>(), "the configfile")
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-c", "file", "-c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfi", "file", "--cfgfi", "anotherfile"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-c", "file", "-c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file", "/c", "anotherfile"}           ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "cfgfile=output.txt\ncfgfile=output.txt\n"}            ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "option '--cfgfile' cannot be specified more than once",
                                "option '--cfgfile' cannot be specified more than once",
                                "option '-c' cannot be specified more than once",
                                "option '/c' cannot be specified more than once",
                                "option 'cfgfile' cannot be specified more than once",
    };
    test_exception_message<multiple_occurrences>(argv, desc, "multiple_occurrences",
                                                expected_msg);
}

//________________________________________________________________________________________
// 
//  unknown_option
//
//________________________________________________________________________________________
void test_unknown_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("good,g",                          "good option") 
    ;

    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-ggc", "file"}      ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--cfgfile", "file"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-ggc", "file"}      ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/c", "file"}        ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "cfgfile=output.txt\n"}        ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {
                                "unrecognised option '-ggc'",
                                "unrecognised option '--cfgfile'",
                                "unrecognised option '-ggc'",
                                "unrecognised option '/c'",
                                "unrecognised option 'cfgfile'",
    };
    test_exception_message<unknown_option>(argv, desc, "unknown_option", expected_msg);
}

//________________________________________________________________________________________
// 
//  validation_error::invalid_bool_value
//
//________________________________________________________________________________________
void test_invalid_bool_value_exception_msg()
{
    options_description desc; 
    desc.add_options()
        ("bool_option,b",	value< bool>(),	"bool_option")
        ;


    vector<vector<const char*>> argv;
    const char* argv0[] = {"program", "-b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = {"program", "--bool_optio", "file"} ; VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = {"program", "-b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = {"program", "/b", "file"}           ; VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "bool_option=output.txt\n"}       ; VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "the argument ('file') for option '--bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '--bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '-b' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('file') for option '/b' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
                                "the argument ('output.txt') for option 'bool_option' is invalid. Valid choices are 'on|off', 'yes|no', '1|0' and 'true|false'",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::invalid_bool_value",
                                expected_msg);
}




//________________________________________________________________________________________
// 
//  validation_error::multiple_values_not_allowed
//
//________________________________________________________________________________________
//
//  Strange exception: sole purpose seems to be catching multitoken() not associated with a vector<string>??
//                   This is surely a corner case.
//
void test_multiple_values_not_allowed_exception_msg()
{
    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<string>()->multitoken(),   "the config file")
         ("good,g",                                     "good option")
         ("output,o", value<string>()->multitoken(),    "the output file")
       ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-c", "file", "c", "-o", "fritz", "hugo" }              ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfgfil", "file", "c", "--outpu", "fritz", "hugo" }   ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-c", "file", "c", "-o", "fritz", "hugo"}               ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/c", "file", "c", "/o", "fritz", "hugo"}               ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "" }                                                               ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "option '--cfgfile' only takes a single argument",
                                "option '--cfgfile' only takes a single argument",
                                "option '-c' only takes a single argument",
                                "option '/c' only takes a single argument",
                                "",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::multiple_values_not_allowed",
                                expected_msg);
}

//________________________________________________________________________________________
// 
//  validation_error::at_least_one_value_required
//
//________________________________________________________________________________________
//
//  Can't build a test case!!!
// 
// 
void test_at_least_one_value_required_exception_msg()
{

    return; 

    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<vector<int>>()->multitoken(), "the config file")
         ("other,o", value<string>(), "other")
       ;

    vector<vector<const char*>> argv;
    const char* argv0[] = { "program", "-c"                          }  ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--cfg"                       }  ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-c"   , "-o"   , "name"      }  ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/c"                          }  ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "" }                          ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "Unexpected exception. the required argument for option '--cfgfile' is missing",
                                "Unexpected exception. the required argument for option '--cfgfile' is missing",
                                "Unexpected exception. the required argument for option '-c' is missing",
                                "Unexpected exception. the required argument for option '/c' is missing",
                                "",
    };
    test_exception_message<validation_error>(argv, 
                                desc,
                                "validation_error::at_least_one_value_required",
                                expected_msg);
}


//________________________________________________________________________________________
// 
//  required_option
//
//________________________________________________________________________________________
void test_required_option_exception_msg()
{
    options_description desc; 
    desc.add_options()
         ("cfgfile,c", value<string>()->required(), "the config file")
         ("good,g",                                 "good option")
         ("output,o", value<string>()->required(),  "the output file")
       ;

    vector<vector<const char*>> argv;            
    const char* argv0[] = { "program", "-g" }    ;      VEC_STR_PUSH_BACK(argv, argv0);
    const char* argv1[] = { "program", "--g" }   ;      VEC_STR_PUSH_BACK(argv, argv1); 
    const char* argv2[] = { "program", "-g"}     ;      VEC_STR_PUSH_BACK(argv, argv2);
    const char* argv3[] = { "program", "/g"}     ;      VEC_STR_PUSH_BACK(argv, argv3); 
    const char* argv4[] = { "" }                 ;      VEC_STR_PUSH_BACK(argv, argv4); 
    const char* expected_msg[5] = {                               
                                "the option '--cfgfile' is required but missing",
                                "the option '--cfgfile' is required but missing",
                                "the option '-c' is required but missing",
                                "the option '/c' is required but missing",
                                "the option 'cfgfile' is required but missing",
    };
    test_exception_message<required_option>(argv, 
                                desc,
                                "required_option",
                                expected_msg);
}


















/** 
 * Check if this is the expected exception with the right message is being thrown inside 
 * func 
*/
template <typename EXCEPTION, typename FUNC>
void test_exception(const string& test_name, const string& exception_txt, FUNC func)
{

    try {
        options_description desc; 
        variables_map vm;
        func(desc, vm);
    } 
    catch (EXCEPTION& e)
    {
       CHECK_EQUAL(test_name, e.what(), exception_txt);
       return;
    }
    catch (std::exception& e)
    {
       BOOST_ERROR(string(test_name + ":\nUnexpected exception. ") + e.what() + 
                   "\nExpected text:\n" + exception_txt + "\n\n");
       return;
    }
    BOOST_ERROR(test_name + ": No exception thrown. ");
}



//________________________________________________________________________________________
// 
//  check_reading_file
//
//________________________________________________________________________________________
void check_reading_file(options_description& desc, variables_map& vm)
{
    desc.add_options()
         ("output,o", value<string>(), "the output file");
    
    const char* file_name = "no_such_file";
    store(parse_config_file<char>(file_name, desc, true), vm);

}


//________________________________________________________________________________________
// 
//  config_file_wildcard
//
//________________________________________________________________________________________
void config_file_wildcard(options_description& desc, variables_map& vm)
{
    desc.add_options()
         ("outpu*", value<string>(), "the output file1")
         ("outp*",    value<string>(), "the output file2")
       ;
    istringstream is("output1=whichone\noutput2=whichone\n");
    store(parse_config_file(is, desc), vm);
}

//________________________________________________________________________________________
// 
//  invalid_syntax::unrecognized_line
//
//________________________________________________________________________________________
void unrecognized_line(options_description& desc, variables_map& vm)
{
    istringstream is("funny wierd line\n");
    store(parse_config_file(is, desc), vm);
}

//________________________________________________________________________________________
// 
//  abbreviated_options_in_config_file
//
//________________________________________________________________________________________
void abbreviated_options_in_config_file(options_description& desc, variables_map& vm)
{
    desc.add_options()(",o", value<string>(), "the output file");
    istringstream is("o=output.txt\n");
    store(parse_config_file(is, desc), vm);
}


//________________________________________________________________________________________
// 
//  too_many_positional_options
//
//________________________________________________________________________________________
void too_many_positional_options(options_description& desc, variables_map& vm)
{
    const char* argv[] = {"program", "1", "2", "3"};
    positional_options_description positional_args;
    positional_args.add("two_positional_arguments", 2);
    store(command_line_parser(4, argv).options(desc).positional(positional_args).run(), vm);
}


//________________________________________________________________________________________
// 
//  invalid_command_line_style
//
//________________________________________________________________________________________

void test_invalid_command_line_style_exception_msg()
{
    string test_name = "invalid_command_line_style";
    using namespace command_line_style;
    unsigned basic_unix_style = (allow_short   
                                  | allow_long 
                                  | allow_sticky | allow_guessing 
                                  | allow_dash_for_short);
    options_description desc; 
    desc.add_options()("output,o", value<string>(), "the output file");

    vector<int> invalid_styles;
    invalid_styles.push_back(allow_short | short_allow_adjacent);
    invalid_styles.push_back(allow_short | allow_dash_for_short);
    invalid_styles.push_back(allow_long);
    vector<string> invalid_diagnostics;
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of command_line_style::allow_slash_for_short "
                                  "(slashes) or command_line_style::allow_dash_for_short "
                                  "(dashes) for short options.");
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of 'command_line_style::short_allow_next' "
                                  "(whitespace separated arguments) or "
                                  "command_line_style::short_allow_adjacent ('=' "
                                  "separated arguments) for short options.");
    invalid_diagnostics.push_back("boost::program_options misconfiguration: choose one "
                                  "or other of 'command_line_style::long_allow_next' "
                                  "(whitespace separated arguments) or "
                                  "command_line_style::long_allow_adjacent ('=' "
                                  "separated arguments) for long options.");


    const char* argv[] = {"program"};
    variables_map vm;
    for (unsigned ii = 0; ii < 3; ++ii)
    {
        bool exception_thrown = false;
        try
        {
            store(parse_command_line(1, argv, desc, invalid_styles[ii]), vm);
        }
        catch (invalid_command_line_style& e)
        {
           string error_msg("arguments are not allowed for unabbreviated option names");
           CHECK_EQUAL(test_name, e.what(), invalid_diagnostics[ii]);
           exception_thrown = true;
        }
        catch (std::exception& e)
        {
           BOOST_ERROR(string(test_name + ":\nUnexpected exception. ") + e.what() + 
                            "\nExpected text:\n" + invalid_diagnostics[ii] + "\n");
           exception_thrown = true;
        }
        if (!exception_thrown)
        {
            BOOST_ERROR(test_name << ": No exception thrown. ");
        }
    }
}


int main(int /*ac*/, char** /*av*/)
{
   test_ambiguous_option_exception_msg();
   test_unknown_option_exception_msg();
   test_multiple_occurrences_exception_msg();
   test_missing_value_exception_msg();
   test_invalid_option_value_exception_msg();
   test_invalid_bool_value_exception_msg();
   test_multiple_values_not_allowed_exception_msg();
   test_required_option_exception_msg();
   test_at_least_one_value_required_exception_msg();


   string test_name;
   string expected_message;


   // check_reading_file
   test_name        = "check_reading_file";
   expected_message = "can not read options configuration file 'no_such_file'";
   test_exception<reading_file>(test_name, expected_message, check_reading_file);

   // config_file_wildcard
   test_name        = "config_file_wildcard";
   expected_message = "options 'outpu*' and 'outp*' will both match the same arguments from the configuration file";
   test_exception<error>(test_name, expected_message, config_file_wildcard);

   // unrecognized_line
   test_name        = "unrecognized_line";
   expected_message = "the options configuration file contains an invalid line 'funny wierd line'";
   test_exception<invalid_syntax>(test_name, expected_message, unrecognized_line);


   // abbreviated_options_in_config_file
   test_name        = "abbreviated_options_in_config_file";
   expected_message = "abbreviated option names are not permitted in options configuration files";
   test_exception<error>(test_name, expected_message, abbreviated_options_in_config_file);

   test_name        = "too_many_positional_options";
   expected_message = "too many positional arguments have been specified on the command line";
   test_exception<too_many_positional_options_error>(
                          test_name, expected_message, too_many_positional_options);

   test_invalid_command_line_style_exception_msg();
   return 0;
}



